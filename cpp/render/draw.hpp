#pragma once
#include <SDL3/SDL.h>

#include "camera.hpp"

namespace galileu::draw {

namespace palette {
inline constexpr SDL_Color kBackground{20, 20, 30, 255};
inline constexpr SDL_Color kPlane{230, 230, 230, 255};
inline constexpr SDL_Color kGuide{120, 120, 140, 255};
inline constexpr SDL_Color kVelocity{90, 230, 130, 255};
inline constexpr SDL_Color kText{255, 255, 255, 255};
inline constexpr SDL_Color kTextDim{190, 190, 205, 255};
inline constexpr SDL_Color kHighlight{255, 220, 120, 255};
inline constexpr SDL_Color kError{255, 110, 110, 255};
} // namespace palette

void SetColor(SDL_Renderer *r, SDL_Color c);

void Line(SDL_Renderer *r, const Camera &cam, Vec2 a, Vec2 b);

// tracejado horizontal ancorado no mundo (não "desliza" com a câmera)
void DashedHLine(SDL_Renderer *r, const Camera &cam, double y, double xmin,
                 double xmax, double dash);

void FillCircle(SDL_Renderer *r, const Camera &cam, Vec2 c, double radius,
                SDL_FColor color);

void ArrowPx(SDL_Renderer *r, SDL_FPoint a, SDL_FPoint b); // em pixels
void Arrow(SDL_Renderer *r, const Camera &cam, Vec2 from, Vec2 to);

// texto ancorado em um ponto do mundo, com deslocamento em pixels
void Text(SDL_Renderer *r, const Camera &cam, Vec2 p, float dx, float dy,
          const char *text);

} // namespace galileu::draw
