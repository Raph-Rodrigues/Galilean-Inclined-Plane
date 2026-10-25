#pragma once
#include <cstddef>
#include <deque>

#include "math.hpp"

namespace galileu {
struct EnergySample {
  double t;
  double kinetic;
  double potential;
};

class History {
public:
  static constexpr double kEnergyWindow = 15.0;

  void Clear() {
    trail_.clear();
    energy_.clear();
  }

  void AddTrail(Vec2 p) {
    if (!trail_.empty()) {
      double dx = p.x - trail_.back().x, dy = p.y - trail_.back().y;
      if (dx * dx + dy * dy < kMinStep * kMinStep) {
        return;
      }
    }
    trail_.push_back(p);
    if (trail_.size() > kMaxTrail)
      trail_.pop_front();
  }

  void AddEnergy(double t, double v, double height, double g) {
    if (!energy_.empty() && t < energy_.back().t + 0.02) {
      return;
    }
    energy_.push_back({t, 0.5 * v * v, g * height});
    while (energy_.front().t < t - kEnergyWindow)
      energy_.pop_front();
  }

  const std::deque<Vec2> &Trail() const { return trail_; }
  const std::deque<EnergySample> &Energy() const { return energy_; }

private:
  static constexpr double kMinStep = 0.04;
  static constexpr size_t kMaxTrail = 600;
  std::deque<Vec2> trail_;
  std::deque<EnergySample> energy_;
};
} // namespace galileu
