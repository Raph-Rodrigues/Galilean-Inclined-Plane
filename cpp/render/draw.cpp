#include "draw.hpp"

#include <cmath>
#include <numbers>
#include <vector>

namespace galileu::draw {

void SetColor(SDL_Renderer *r, SDL_Color c) {
  SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

void Line(SDL_Renderer *r, const Camera &cam, Vec2 a, Vec2 b) {
  SDL_FPoint pa = cam.ToScreen(a), pb = cam.ToScreen(b);
  SDL_RenderLine(r, pa.x, pa.y, pb.x, pb.y);
}

void DashedHLine(SDL_Renderer *r, const Camera &cam, double y, double xmin,
                 double xmax, double dash) {
  double period = 2.0 * dash;
  for (double x = std::floor(xmin / period) * period; x < xmax; x += period)
    Line(r, cam, {x, y}, {x + dash, y});
}

void FillCircle(SDL_Renderer *r, const Camera &cam, Vec2 c, double radius,
                SDL_FColor color) {
  const int seg = 32;
  SDL_FPoint center = cam.ToScreen(c);
  float rp = (float)(radius * cam.scale);
  std::vector<SDL_Vertex> v;
  v.reserve(seg * 3);
  for (int i = 0; i < seg; ++i) {
    float a0 = 2.0f * (float)std::numbers::pi * i / seg;
    float a1 = 2.0f * (float)std::numbers::pi * (i + 1) / seg;
    v.push_back(SDL_Vertex{center, color, {0, 0}});
    v.push_back(
        SDL_Vertex{{center.x + rp * std::cos(a0), center.y + rp * std::sin(a0)},
                   color,
                   {0, 0}});
    v.push_back(
        SDL_Vertex{{center.x + rp * std::cos(a1), center.y + rp * std::sin(a1)},
                   color,
                   {0, 0}});
  }
  SDL_RenderGeometry(r, nullptr, v.data(), (int)v.size(), nullptr, 0);
}

// seta em coordenadas de tela, duas "barbas" de 25 graus na ponta
void ArrowPx(SDL_Renderer *r, SDL_FPoint a, SDL_FPoint b) {
  SDL_RenderLine(r, a.x, a.y, b.x, b.y);
  float dx = b.x - a.x, dy = b.y - a.y;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 6.0f)
    return; // curta demais para ter ponta
  float ux = dx / len, uy = dy / len;
  const float head = 10.0f, c = 0.9063f, s = 0.4226f; // cos/sen de 25°
  SDL_RenderLine(r, b.x, b.y, b.x - head * (ux * c - uy * s),
                 b.y - head * (ux * s + uy * c));
  SDL_RenderLine(r, b.x, b.y, b.x - head * (ux * c + uy * s),
                 b.y - head * (uy * c - ux * s));
}

void Arrow(SDL_Renderer *r, const Camera &cam, Vec2 from, Vec2 to) {
  ArrowPx(r, cam.ToScreen(from), cam.ToScreen(to));
}

void Text(SDL_Renderer *r, const Camera &cam, Vec2 p, float dx, float dy,
          const char *text) {
  SDL_FPoint s = cam.ToScreen(p);
  SDL_RenderDebugText(r, s.x + dx, s.y + dy, text);
}

} // namespace galileu::draw
