#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "core/math.hpp"
#include "render/draw.hpp"
#include "render/scene_renderer.hpp"

namespace fs = std::filesystem;

namespace galileu {
namespace {

const std::string kConfigPath = GALILEU_SCRIPTS_DIR "/config.lua";
constexpr double kPhysicsDt = 1.0 / 120.0;

std::vector<double> AnglesFromConfig(const Config &cfg) {
  return cfg.angles.empty() ? std::vector<double>{cfg.theta2} : cfg.angles;
}

} // namespace

// ---------------------------------------------------------------- Load -----
bool Game::Load() {
  std::string err;
  if (!script_.LoadConfig(kConfigPath, cfg_, &err)) {
    fprintf(stderr, "[Game] nao foi possivel carregar %s: %s\n",
            kConfigPath.c_str(), err.c_str());
    return false;
  }
  printf("[LUA] %s (%dx%d)\n", cfg_.title.c_str(), cfg_.width, cfg_.height);

  seq_.SetAngles(AnglesFromConfig(cfg_));
  if (!sim_.Start(cfg_.g, cfg_.h0, deg2rad(cfg_.theta1),
                  deg2rad(seq_.CurrentAngle()), kPhysicsDt)) {
    fprintf(stderr, "[Game] falha ao iniciar a thread de fisica\n");
    return false;
  }

  cam_.scale = cfg_.scale;
  std::error_code ec;
  cfgStamp_ = fs::last_write_time(kConfigPath, ec);
  return true;
}

void Game::Unload() { sim_.Stop(); }

WindowSpec Game::Window() const {
  return {cfg_.title, cfg_.width, cfg_.height};
}

std::optional<WindowSpec> Game::TakeWindowChange() {
  if (!windowDirty_)
    return std::nullopt;
  windowDirty_ = false;
  return Window();
}

void Game::Resize(int w, int h) {
  cam_.w = w;
  cam_.h = h;
}

// --------------------------------------------------------------- Eventos ---
void Game::OnEvent(const SDL_Event &e) {
  if (e.type != SDL_EVENT_KEY_DOWN)
    return;
  const size_t n = seq_.Count();
  switch (e.key.key) {
  case SDLK_ESCAPE:
    quit_ = true;
    break;
  case SDLK_RIGHT:
    BeginSituation(seq_.Index() + 1);
    break;
  case SDLK_LEFT:
    BeginSituation(seq_.Index() + n - 1);
    break;
  case SDLK_R:
    BeginSituation(seq_.Index());
    break;
  case SDLK_F5:
  case SDLK_L: {
    ReloadConfig();
    std::error_code ec;
    cfgStamp_ = fs::last_write_time(kConfigPath, ec); // evita recarregar 2x
    break;
  }
  default:
    break;
  }
}

void Game::BeginSituation(size_t index) {
  seq_.Begin(index);
  sim_.Reset(cfg_.g, cfg_.h0, deg2rad(cfg_.theta1),
             deg2rad(seq_.CurrentAngle()));
}

// ------------------------------------------------------------- Hot reload --
void Game::ReloadConfig() {
  const Config old = cfg_;
  std::string err;
  if (!script_.LoadConfig(kConfigPath, cfg_, &err)) {
    // cfg_ não foi tocada: a simulação segue com os valores anteriores
    status_ =
        StatusMessage{"ERRO no config.lua (valores anteriores mantidos): " +
                          err.substr(0, 100),
                      false, 6.0};
    return;
  }
  if (cfg_.title != old.title || cfg_.width != old.width ||
      cfg_.height != old.height)
    windowDirty_ = true;

  if (!old.SamePhysics(cfg_)) {
    seq_.SetAngles(AnglesFromConfig(cfg_));
    BeginSituation(seq_.Index()); // reinicia com g, h0, theta1 novos
    status_ = StatusMessage{"config.lua recarregado (simulacao reiniciada)",
                            true, 3.0};
  } else {
    status_ = StatusMessage{"config.lua recarregado", true, 3.0};
  }
}

void Game::PollHotReload(double dt) {
  pollTimer_ += dt;
  if (pollTimer_ < 0.5)
    return;
  pollTimer_ = 0.0;
  std::error_code ec;
  auto stamp = fs::last_write_time(kConfigPath, ec);
  if (!ec && stamp != cfgStamp_) {
    cfgStamp_ = stamp;
    ReloadConfig();
  }
}

// ---------------------------------------------------------------- Update ---
void Game::Update(double dt) {
  dt = std::min(dt, 0.25);
  if (dt > 0.0)
    fps_ = (fps_ == 0.0) ? 1.0 / dt : fps_ * 0.95 + (1.0 / dt) * 0.05;
  status_.timer -= dt;
  PollHotReload(dt);

  SimSnapshot snap = sim_.Read();
  if (seq_.GetPhase() == Phase::Running) {
    if (snap.latest.apex) // a thread parou a bola no ponto mais alto
      seq_.OnApex(snap.latest.s, snap.latest.height);
  } else if (seq_.TickHold(dt)) {
    BeginSituation(seq_.Index() + 1);
    snap = sim_.Read(); // estado novo, já da próxima situação
  }

  BuildModel(snap);
  UpdateCamera(dt);
}

// Geometria da situação atual + posição da bola (centro deslocado pela normal)
void Game::BuildModel(const SimSnapshot &snap) {
  const double H = cfg_.h0;
  const double theta1 = deg2rad(cfg_.theta1);
  const double theta2Deg = seq_.CurrentAngle();
  const double theta2 = deg2rad(theta2Deg);
  const bool horizontal = seq_.IsHorizontal();

  SceneModel &m = model_;
  m.st = snap.state;
  m.latest = snap.latest;
  m.theta2Deg = theta2Deg;
  m.horizontal = horizontal;

  m.valley = {0.0, 0.0};
  m.leftTop = {-H / std::tan(theta1), H};
  double run2 = horizontal ? cfg_.maxRun
                           : std::min(H / std::sin(theta2),
                                      cfg_.maxRun / std::cos(theta2));
  m.rightEnd = {run2 * std::cos(theta2), run2 * std::sin(theta2)};

  Vec2 n{-m.st.ty, m.st.tx}; // normal do plano (aponta "para cima")
  m.ball = {m.st.x + cfg_.radius * n.x, m.st.y + cfg_.radius * n.y};
}

// Enquadra a cena (planos inclinados) ou segue a bola (plano horizontal)
void Game::UpdateCamera(double dt) {
  double targetScale, targetX;
  if (model_.horizontal) {
    targetScale = cfg_.scale;
    targetX = std::max(0.0, model_.ball.x + 3.0);
  } else {
    double xl = model_.leftTop.x - 1.5, xr = model_.rightEnd.x + 1.5;
    targetScale = std::min(cfg_.scale, cam_.w * 0.92 / (xr - xl));
    targetX = 0.5 * (xl + xr);
  }
  double k = camInit_ ? 1.0 - std::exp(-4.0 * dt) : 1.0;
  camInit_ = true;
  cam_.scale += (targetScale - cam_.scale) * k;
  cam_.center.x += (targetX - cam_.center.x) * k;
  cam_.center.y = cfg_.h0 * 0.5;

  // plano horizontal é infinito: vai até a borda da tela
  if (model_.horizontal)
    model_.rightEnd = {cam_.XMax() + 1.0, 0.0};
}

// ------------------------------------------------------------------ Draw ---
void Game::Draw(SDL_Renderer *r) {
  draw::SetColor(r, draw::palette::kBackground);
  SDL_RenderClear(r);
  render::DrawScene(r, cam_, cfg_, model_);
  render::DrawHud(r, cam_, cfg_, model_, seq_, status_, fps_);
}

} // namespace galileu
