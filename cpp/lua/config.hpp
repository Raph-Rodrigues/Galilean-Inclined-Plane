#pragma once
#include <string>
#include <vector>

namespace galileu {

struct ColorRgb {
  float r = 1.0f, g = 0.6f, b = 0.2f; // 0..1
};

// Dados puros vindos do config.lua (valores padrão = os da versão anterior).
struct Config {
  std::string title = "Plano Inclinado de Galileu";
  int width = 1280, height = 720;

  double g = 9.81, h0 = 5.0, theta1 = 45.0, theta2 = 30.0; // graus

  double radius = 0.3;
  ColorRgb ballColor;

  double scale = 40.0, velScale = 0.5, maxRun = 70.0;

  std::vector<double> angles; // graus

  // true se nenhum parâmetro que exige reiniciar a simulação mudou
  bool SamePhysics(const Config &o) const {
    return g == o.g && h0 == o.h0 && theta1 == o.theta1 && theta2 == o.theta2 &&
           angles == o.angles;
  }
};

} // namespace galileu
