#pragma once
#include "rust_api.h"

namespace galileu {

struct SimSnapshot {
  PhysicsState state; // interpolado: para DESENHAR
  PhysicsState
      latest; // último passo publicado: para a LÓGICA (apex, contadores)
};

// Wrapper RAII sobre a thread de física em Rust.
// Ângulos em radianos.
class Simulation {
public:
  Simulation() = default;
  ~Simulation();
  Simulation(const Simulation &) = delete;
  Simulation &operator=(const Simulation &) = delete;

  bool Start(double g, double h0, double theta1, double theta2, double dt);
  void Reset(double g, double h0, double theta1, double theta2);
  SimSnapshot Read() const;
  void Stop();

private:
  PhysicsEngine *engine_ = nullptr;
};

} // namespace galileu
