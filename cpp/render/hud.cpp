#include "hud.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "draw.hpp"

namespace galileu::render {
namespace {

using namespace draw;

// |v| em texto 2x
void SpeedReadout(SDL_Renderer *r, const Camera &cam, const SceneModel &m) {
  char buf[64];
  SetColor(r, palette::kVelocity);
  SDL_SetRenderScale(r, 2.0f, 2.0f);
  snprintf(buf, sizeof buf, "|v| = %.2f m/s", std::fabs(m.st.v));
  SDL_RenderDebugText(r, 10.0f / 2.0f, (cam.h - 80.0f) / 2.0f, buf);
  SDL_SetRenderScale(r, 1.0f, 1.0f);
}

// descrição da situação atual (rodapé)
void SituationText(SDL_Renderer *r, const Camera &cam, const Config &cfg,
                   const SceneModel &m, const Sequence &seq) {
  char buf[200];
  const float y1 = cam.h - 46.0f, y2 = cam.h - 32.0f;
  SetColor(r, palette::kText);
  if (m.horizontal) {
    SDL_RenderDebugText(
        r, 10, y1,
        "Plano 2 horizontal: sem forca ao longo do movimento -> MRU (inercia)");
    snprintf(buf, sizeof buf,
             "Distancia percorrida no plano 2: %.1f m, e a bola nao para. "
             "Seta direita: recomeca a sequencia",
             std::max(m.st.s, 0.0));
    SDL_RenderDebugText(r, 10, y2, buf);
  } else if (seq.GetPhase() == Phase::Holding) {
    const Result &res = seq.CurrentResult();
    snprintf(buf, sizeof buf,
             "A bola subiu ate h = %.3f m (h0 = %.3f m), percorrendo %.2f m no "
             "plano 2",
             res.height, cfg.h0, res.distance);
    SDL_RenderDebugText(r, 10, y1, buf);
    snprintf(buf, sizeof buf, "Proxima situacao em %.1f s", seq.HoldTimer());
    SDL_RenderDebugText(r, 10, y2, buf);
  } else {
    snprintf(buf, sizeof buf,
             "Plano 2 a %.1f graus: a bola desce o plano 1 e sobe o plano 2",
             m.theta2Deg);
    SDL_RenderDebugText(r, 10, y1, buf);
  }
}

// caixa de legenda (canto superior direito)
void Legend(SDL_Renderer *r, const Camera &cam, const Config &cfg) {
  float lx = cam.w - 250.0f, ly = 12.0f;

  SetColor(r, palette::kPlane);
  SDL_RenderLine(r, lx, ly + 4, lx + 24, ly + 4);
  SDL_RenderDebugText(r, lx + 32, ly, "plano inclinado");

  ly += 16;
  SetColor(r, palette::kGuide);
  SDL_RenderLine(r, lx, ly + 4, lx + 10, ly + 4);
  SDL_RenderLine(r, lx + 16, ly + 4, lx + 24, ly + 4);
  SDL_RenderDebugText(r, lx + 32, ly, "altura inicial h0");

  ly += 16;
  SDL_SetRenderDrawColorFloat(r, cfg.ballColor.r, cfg.ballColor.g,
                              cfg.ballColor.b, 1.0f);
  SDL_FRect sw{lx + 6, ly, 12, 8};
  SDL_RenderFillRect(r, &sw);
  SDL_RenderDebugText(r, lx + 32, ly, "bola");

  ly += 16;
  SetColor(r, palette::kVelocity);
  ArrowPx(r, {lx, ly + 4}, {lx + 24, ly + 4});
  SDL_RenderDebugText(r, lx + 32, ly, "vetor velocidade");
}

// HUD numérico (canto superior esquerdo)
void Numbers(SDL_Renderer *r, const Config &cfg, const SceneModel &m,
             const Sequence &seq, double fps) {
  char buf[200];
  SetColor(r, palette::kText);
  snprintf(buf, sizeof buf,
           "Situacao %zu/%zu   theta1 = %.1f   theta2 = %.1f deg",
           seq.Index() + 1, seq.Count(), cfg.theta1, m.theta2Deg);
  SDL_RenderDebugText(r, 10, 10, buf);
  snprintf(
      buf, sizeof buf,
      "t = %.2f s   passos = %llu   fisica: 120 Hz (thread)   render: %.0f FPS",
      m.latest.time, (unsigned long long)m.latest.steps, fps);
  SDL_RenderDebugText(r, 10, 24, buf);
  snprintf(buf, sizeof buf, "s = %7.3f m   v = %7.3f m/s   h = %6.3f m", m.st.s,
           m.st.v, m.st.height);
  SDL_RenderDebugText(r, 10, 38, buf);
  snprintf(buf, sizeof buf, "E/m = %.6f J/kg", m.st.energy);
  SDL_RenderDebugText(r, 10, 52, buf);
  SDL_RenderDebugText(r, 10, 66,
                      "Setas: situacao | R: reinicia | F5/L: recarrega "
                      "config.lua (auto ao salvar) | ESC: sair");
}

// distância percorrida no plano 2, por situação
void ResultsTable(SDL_Renderer *r, const SceneModel &m, const Sequence &seq) {
  char buf[120];
  SetColor(r, palette::kText);
  SDL_RenderDebugText(r, 10, 90, "Distancia percorrida no plano 2:");

  const auto &angles = seq.Angles();
  const auto &results = seq.Results();
  for (size_t i = 0; i < angles.size(); ++i) {
    const Result &res = results[i];
    const bool cur = (i == seq.Index());
    SetColor(r, cur ? palette::kHighlight : palette::kTextDim);
    if (res.done)
      snprintf(buf, sizeof buf, "%c %5.1f graus: %6.2f m  (h = %.3f m)",
               cur ? '>' : ' ', angles[i], res.distance, res.height);
    else if (cur && m.horizontal)
      snprintf(buf, sizeof buf, "> %5.1f graus: %6.1f m ... (sem fim)",
               angles[i], std::max(m.st.s, 0.0));
    else if (cur)
      snprintf(buf, sizeof buf, "> %5.1f graus: em andamento", angles[i]);
    else
      snprintf(buf, sizeof buf, "  %5.1f graus: --", angles[i]);
    SDL_RenderDebugText(r, 10, 104.0f + 14.0f * (float)i, buf);
  }
}

void Status(SDL_Renderer *r, const Camera &cam, const StatusMessage &s) {
  if (s.timer <= 0.0)
    return;
  SetColor(r, s.ok ? palette::kVelocity : palette::kError);
  SDL_RenderDebugText(r, 10, cam.h - 14.0f, s.text.c_str());
}

} // namespace

void DrawHud(SDL_Renderer *r, const Camera &cam, const Config &cfg,
             const SceneModel &m, const Sequence &seq,
             const StatusMessage &status, double fps) {
  SpeedReadout(r, cam, m);
  SituationText(r, cam, cfg, m, seq);
  Legend(r, cam, cfg);
  Numbers(r, cfg, m, seq, fps);
  ResultsTable(r, m, seq);
  Status(r, cam, status);
}

} // namespace galileu::render
