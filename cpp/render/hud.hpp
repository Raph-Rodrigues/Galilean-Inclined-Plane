#pragma once
#include <SDL3/SDL.h>

#include <string>

#include "../game/sequence.hpp"
#include "../lua/config.hpp"
#include "camera.hpp"
#include "scene_model.hpp"

namespace galileu {

// Mensagem temporária no rodapé (ex.: resultado do hot reload)
struct StatusMessage {
  std::string text;
  bool ok = true;
  double timer = 0.0; // segundos restantes
};

namespace render {

// Interface em coordenadas de tela: módulo da velocidade, descrição da
// situação, legenda, números, tabela de resultados e mensagem de status.
void DrawHud(SDL_Renderer *r, const Camera &cam, const Config &cfg,
             const SceneModel &m, const Sequence &seq,
             const StatusMessage &status, double fps);

} // namespace render
} // namespace galileu
