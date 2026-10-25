#include "render/energy_plot.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "render/draw.hpp"

namespace galileu::render {

void DrawEnergyPlot(SDL_Renderer *r, const Camera &cam, const Config &cfg,
                    const History &hist) {
  using namespace draw;
  constexpr SDL_Color kKinetic{255, 170, 60, 255};
  constexpr SDL_Color kPotential{90, 170, 255, 255};

  const float pw = 300.0f, ph = 120.0f;
  const float px = cam.w - pw - 12.0f, py = cam.h - ph - 12.0f;
  const SDL_FRect box{px, py, pw, ph};

  // fundo translúcido + borda
  SDL_SetRenderDrawColor(r, 10, 10, 18, 200);
  SDL_RenderFillRect(r, &box);
  SetColor(r, palette::kGuide);
  SDL_RenderRect(r, &box);

  // área do gráfico
  const float L = px + 8.0f, R = px + pw - 8.0f;
  const float T = py + 22.0f, B = py + ph - 20.0f;
  const double etot = cfg.g * cfg.h0;

  const auto &s = hist.Energy();
  const double t1 = s.empty() ? 0.0 : s.back().t;
  const double tmax = std::max(t1, History::kEnergyWindow);
  const double tmin = tmax - History::kEnergyWindow;

  auto X = [&](double t) {
    return L + (float)((t - tmin) / (tmax - tmin)) * (R - L);
  };
  auto Y = [&](double e) {
    float f = (float)std::clamp(e / (etot * 1.05), 0.0, 1.0);
    return B - f * (B - T);
  };

  // título e janela de tempo
  char buf[64];
  SetColor(r, palette::kText);
  SDL_RenderDebugText(r, px + 8, py + 6, "Energia por unidade de massa");
  snprintf(buf, sizeof buf, "t:%.0f-%.0fs", tmin, tmax);
  SDL_RenderDebugText(
      r, px + pw - 8.0f - 8.0f * (float)std::char_traits<char>::length(buf),
      py + 6, buf);

  // eixo x (energia zero) e linha da energia total
  SetColor(r, palette::kGuide);
  SDL_RenderLine(r, L, B, R, B);
  SetColor(r, palette::kPlane);
  SDL_RenderLine(r, L, Y(etot), R, Y(etot));

  // curvas K e U
  if (s.size() >= 2) {
    std::vector<SDL_FPoint> k, u;
    k.reserve(s.size());
    u.reserve(s.size());
    for (const EnergySample &e : s) {
      k.push_back({X(e.t), Y(e.kinetic)});
      u.push_back({X(e.t), Y(e.potential)});
    }
    SetColor(r, kKinetic);
    SDL_RenderLines(r, k.data(), (int)k.size());
    SetColor(r, kPotential);
    SDL_RenderLines(r, u.data(), (int)u.size());
  }

  // legenda das curvas
  const float ly = py + ph - 14.0f;
  SetColor(r, kKinetic);
  SDL_RenderDebugText(r, px + 8, ly, "cinetica");
  SetColor(r, kPotential);
  SDL_RenderDebugText(r, px + 84, ly, "potencial");
  SetColor(r, palette::kPlane);
  snprintf(buf, sizeof buf, "total=%.1f", etot);
  SDL_RenderDebugText(r, px + 172, ly, buf);
}

} // namespace galileu::render
