#pragma once
#include <SDL3/SDL.h>

#include "../core/math.hpp"

namespace galileu {

// Câmera: metros (y para cima) -> pixels (y para baixo)
struct Camera {
  double scale = 40.0;   // pixels por metro
  Vec2 center{0.0, 2.5}; // ponto do mundo no centro da tela
  int w = 1280, h = 720;

  SDL_FPoint ToScreen(Vec2 p) const {
    return {(float)(w * 0.5 + (p.x - center.x) * scale),
            (float)(h * 0.5 - (p.y - center.y) * scale)};
  }
  double HalfWidth() const { return w * 0.5 / scale; }
  double XMin() const { return center.x - HalfWidth(); }
  double XMax() const { return center.x + HalfWidth(); }
};

} // namespace galileu
