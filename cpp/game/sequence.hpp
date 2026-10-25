#pragma once
#include <cstddef>
#include <vector>

namespace galileu {

enum class Phase { Running, Holding };

struct Result {
  bool done = false;
  double distance = 0.0; // s no ponto mais alto do plano 2
  double height = 0.0;
};

// Máquina de estados das situações (ângulo do plano 2).
// Running: a simulação corre | Holding: parada no ponto mais alto, esperando.
class Sequence {
public:
  explicit Sequence(double holdTime = 2.0) : holdTime_(holdTime) {}

  void SetAngles(std::vector<double> anglesDeg); // limita a [0, 85]
  void Begin(size_t index);                      // índice circular
  void OnApex(double distance, double height);   // Running -> Holding
  bool TickHold(double dt);                      // true quando a pausa acabou

  double CurrentAngle() const { return angles_[index_]; }
  bool IsHorizontal() const { return CurrentAngle() < 0.5; }
  size_t Index() const { return index_; }
  size_t Count() const { return angles_.size(); }
  Phase GetPhase() const { return phase_; }
  double HoldTimer() const { return holdTimer_; }
  const std::vector<double> &Angles() const { return angles_; }
  const std::vector<Result> &Results() const { return results_; }
  const Result &CurrentResult() const { return results_[index_]; }

private:
  std::vector<double> angles_;
  std::vector<Result> results_;
  size_t index_ = 0;
  Phase phase_ = Phase::Running;
  double holdTime_;
  double holdTimer_ = 0.0;
};

} // namespace galileu
