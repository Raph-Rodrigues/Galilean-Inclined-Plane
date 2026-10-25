#pragma once
#include <SDL3/SDL.h>

#include "core/history.hpp"
#include "lua/config.hpp"
#include "render/camera.hpp"

namespace galileu::render {
// Gráfico de energia (por unidade de massa) no canto inferior direito:
// cinética K, potencial U e total E, em janela deslizante de 15 s.

void DrawEnergyPlot(SDL_Renderer *r, const Camera &cam, const Config &cfg,
                    const History &hist);
} // namespace galileu::render
