#pragma once

extern "C" {

// Deve ter exatamente o mesmo layout de PhysicsState em physics/src/lib.rs
struct PhysicsState {
  double s, v;
  double x, y;
  double tx, ty; // tangente unitária (direção de s crescente)
  double speed;
  double height;
  double energy; // v²/2 + g*h (por unidade de massa)
  double time;
};

struct PhysicsWorld; // opaco: só o Rust conhece o conteúdo

PhysicsWorld *physics_create(double g, double h0, double theta1, double theta2);
void physics_destroy(PhysicsWorld *w);
void physics_reset(PhysicsWorld *w, double g, double h0, double theta1,
                   double theta2);
void physics_step(PhysicsWorld *w, double dt);
void physics_get_state(const PhysicsWorld *w, PhysicsState *out);

} // extern "C"
