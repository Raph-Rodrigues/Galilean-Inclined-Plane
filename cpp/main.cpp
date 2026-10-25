#include <SDL3/SDL_main.h>

#include "app/app.hpp"

int main(int, char **) {
  galileu::App app;
  return app.Run();
}
