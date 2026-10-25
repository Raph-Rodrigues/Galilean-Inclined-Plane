#pragma once
#include <SDL3/SDL.h>

#include "core/history.hpp"
#include "lua/config.hpp"
#include "render/camera.hpp"
#include "render/scene_model.hpp"

namespace galileu::render {

// Mundo: linha de altura, planos, rastro, bola, vetor velocidade e rótulos.
void DrawScene(SDL_Renderer *r, const Camera &cam, const Config &cfg,
               const SceneModel &m, const History &hist,
               const ViewOptions &opts);

} // namespace galileu::render
