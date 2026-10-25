#pragma once
#include "core/math.hpp"
#include "physics/rust_api.h"

namespace galileu {

// Tudo que o renderer precisa para desenhar um frame.
// Calculado em Game::Update; o renderer só LÊ.
struct SceneModel {
  PhysicsState st{}; // estado interpolado (posição/velocidade para desenhar)
  PhysicsState latest{}; // último passo publicado (contadores no HUD)
  Vec2 valley{0, 0}, leftTop{0, 0}, rightEnd{0, 0}, ball{0, 0};
  double theta2Deg = 0.0;
  bool horizontal = false;
  bool paused = false;
  double speed = 1.0; // multiplicador do tempo simulado
};

// Opções de visualização (teclas T e G)
struct ViewOptions {
  bool trail = true;
  bool plot = true;
};

} // namespace galileu
