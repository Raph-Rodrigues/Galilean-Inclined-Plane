#pragma once
#include <numbers>

namespace galileu {

struct Vec2 {
  double x, y;
};

constexpr double deg2rad(double d) { return d * std::numbers::pi / 180.0; }

} // namespace galileu
