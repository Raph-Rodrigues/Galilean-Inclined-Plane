#include "simulation.hpp"
#include "rust_api.h"

namespace galileu {
namespace {

// mistura dois snapshots: t = 0 -> a, t = 1 -> b
PhysicsState Interpolate(const PhysicsState &a, const PhysicsState &b,
                         double t) {
  auto lerp = [t](double p, double q) { return p + (q - p) * t; };
  PhysicsState o = b;
  o.s = lerp(a.s, b.s);
  o.v = lerp(a.v, b.v);
  o.x = lerp(a.x, b.x);
  o.y = lerp(a.y, b.y);
  o.speed = lerp(a.speed, b.speed);
  o.height = lerp(a.height, b.height);
  o.energy = lerp(a.energy, b.energy);
  o.time = lerp(a.time, b.time);
  // a tangente muda de repente no vale: usa a do snapshot mais próximo
  if (t < 0.5) {
    o.tx = a.tx;
    o.ty = a.ty;
  }
  return o;
}

} // namespace

Simulation::~Simulation() { Stop(); }

bool Simulation::Start(double g, double h0, double theta1, double theta2,
                       double dt) {
  Stop();
  engine_ = physics_create(g, h0, theta1, theta2, dt);
  return engine_ != nullptr;
}

void Simulation::Reset(double g, double h0, double theta1, double theta2) {
  if (engine_)
    physics_reset(engine_, g, h0, theta1, theta2);
}

void Simulation::SetPaused(bool paused) {
  if (engine_) {
    physics_set_paused(engine_, paused);
  }
}

void Simulation::SetSpeed(double speed) {
  if (engine_) {
    physics_set_speed(engine_, speed);
  }
}

SimSnapshot Simulation::Read() const {
  SimSnapshot out{};
  if (!engine_)
    return out;
  PhysicsState prev{}, curr{};
  double alpha = 1.0;
  physics_get_snapshots(engine_, &prev, &curr, &alpha);
  out.state = Interpolate(prev, curr, alpha);
  out.latest = curr;
  return out;
}

void Simulation::Stop() {
  if (engine_) {
    physics_destroy(engine_); // para a thread e espera o join
    engine_ = nullptr;
  }
}

} // namespace galileu
