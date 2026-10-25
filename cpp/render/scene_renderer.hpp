#pragma once
#include <SDL3/SDL.h>

#include "../lua/config.hpp"
#include "camera.hpp"
#include "scene_model.hpp"

namespace galileu::render {

// Mundo: linha de altura, planos, bola, vetor velocidade e seus rótulos.
void DrawScene(SDL_Renderer *r, const Camera &cam, const Config &cfg,
               const SceneModel &m);

} // namespace galileu::render
