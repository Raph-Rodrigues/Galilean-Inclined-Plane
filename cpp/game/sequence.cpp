#include "sequence.hpp"

#include <algorithm>
#include <utility>

namespace galileu {

void Sequence::SetAngles(std::vector<double> anglesDeg) {
  angles_ = std::move(anglesDeg);
  if (angles_.empty())
    angles_.push_back(30.0);
  for (double &a : angles_)
    a = std::clamp(a, 0.0, 85.0);
  results_.assign(angles_.size(), Result{});
  index_ = std::min(index_, angles_.size() - 1);
  phase_ = Phase::Running;
  holdTimer_ = 0.0;
}

void Sequence::Begin(size_t index) {
  index_ = index % angles_.size();
  phase_ = Phase::Running;
  holdTimer_ = 0.0;
  results_[index_] = Result{};
}

void Sequence::OnApex(double distance, double height) {
  phase_ = Phase::Holding;
  holdTimer_ = holdTime_;
  results_[index_] = Result{true, distance, height};
}

bool Sequence::TickHold(double dt) {
  if (phase_ != Phase::Holding)
    return false;
  holdTimer_ -= dt;
  return holdTimer_ <= 0.0;
}

} // namespace galileu
