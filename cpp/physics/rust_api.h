#pragma once
#include <cstdint>

// API C exportada por physics/src/ffi.rs.
// Este arquivo e physics/src/state.rs devem ter o mesmo layout.
extern "C" {

struct PhysicsState {
  double s, v;
  double x, y;
  double tx, ty; // tangente unitária (direção de s crescente)
  double speed;
  double height;
  double energy; // v²/2 + g*h (por unidade de massa)
  double time;
  uint64_t steps; // passos de física desde o reset
  uint32_t apex;  // 1 = parou no ponto mais alto (simulação congelada)
  uint32_t _pad;
};

struct PhysicsEngine; // opaco: dono da thread de física

PhysicsEngine *physics_create(double g, double h0, double theta1, double theta2,
                              double dt);
void physics_destroy(PhysicsEngine *e); // para a thread e libera
void physics_reset(PhysicsEngine *e, double g, double h0, double theta1,
                   double theta2);
void physics_get_snapshots(const PhysicsEngine *e, PhysicsState *prev,
                           PhysicsState *curr, double *alpha);

} // extern "C"
