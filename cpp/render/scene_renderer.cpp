#include "render/scene_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "render/draw.hpp"

namespace galileu::render {
namespace {

// rastro: some aos poucos (mais antigo = mais transparente) e termina na bola
void DrawTrail(SDL_Renderer *r, const Camera &cam, const Config &cfg,
               const SceneModel &m, const History &hist) {
  const auto &t = hist.Trail();
  if (t.empty())
    return;
  const size_t n = t.size();
  const Uint8 cr = (Uint8)(cfg.ballColor.r * 255.0f);
  const Uint8 cg = (Uint8)(cfg.ballColor.g * 255.0f);
  const Uint8 cb = (Uint8)(cfg.ballColor.b * 255.0f);

  SDL_FPoint prev = cam.ToScreen(t[0]);
  for (size_t i = 1; i <= n; ++i) {
    SDL_FPoint cur = cam.ToScreen(i < n ? t[i] : m.ball);
    Uint8 a = (Uint8)(20.0 + 150.0 * (double)i / (double)n);
    SDL_SetRenderDrawColor(r, cr, cg, cb, a);
    SDL_RenderLine(r, prev.x, prev.y, cur.x, cur.y);
    prev = cur;
  }
}

} // namespace

void DrawScene(SDL_Renderer *r, const Camera &cam, const Config &cfg,
               const SceneModel &m, const History &hist,
               const ViewOptions &opts) {
  using namespace draw;
  char buf[160];
  const double H = cfg.h0;
  const double xmin = cam.XMin(), xmax = cam.XMax();

  // linha de altura de referência
  SetColor(r, palette::kGuide);
  DashedHLine(r, cam, H, xmin, xmax, 12.0 / cam.scale);
  snprintf(buf, sizeof buf, "altura inicial h0 = %.1f m", H);
  Text(r, cam, {xmin, H}, 8, -14, buf);

  // planos + legendas
  SetColor(r, palette::kPlane);
  Line(r, cam, m.leftTop, m.valley);
  Line(r, cam, m.valley, m.rightEnd);
  snprintf(buf, sizeof buf, "Plano 1: %.1f graus", cfg.theta1);
  Text(r, cam, {m.leftTop.x * 0.5, m.leftTop.y * 0.5}, -140, 6, buf);
  snprintf(buf, sizeof buf, "Plano 2: %.1f graus", m.theta2Deg);
  Text(r, cam,
       {std::clamp(m.rightEnd.x * 0.5, xmin + 4.0, xmax - 12.0),
        m.rightEnd.y * 0.5},
       -40, 16, buf);

  // rastro (por baixo da bola)
  if (opts.trail)
    DrawTrail(r, cam, cfg, m, hist);

  // bola
  FillCircle(
      r, cam, m.ball, cfg.radius,
      SDL_FColor{cfg.ballColor.r, cfg.ballColor.g, cfg.ballColor.b, 1.0f});

  // vetor velocidade (v com sinal * tangente) + módulo ao lado da seta
  Vec2 vvec{m.st.v * m.st.tx, m.st.v * m.st.ty};
  Vec2 tip{m.ball.x + vvec.x * cfg.velScale, m.ball.y + vvec.y * cfg.velScale};
  SetColor(r, palette::kVelocity);
  Arrow(r, cam, m.ball, tip);
  snprintf(buf, sizeof buf, "v = %.2f m/s", std::fabs(m.st.v));
  Text(r, cam, tip, 8, -14, buf);
}

} // namespace galileu::render
