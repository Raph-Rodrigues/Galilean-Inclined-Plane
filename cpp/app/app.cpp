#include "app.hpp"

#include <SDL3/SDL.h>

#include <cstdio>

#include "../game/game.hpp"

namespace galileu {

int App::Run() {
  Game game;
  if (!game.Load()) // lê o Lua e cria a simulação (ainda sem janela)
    return 1;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    return 1;
  }

  const WindowSpec spec = game.Window();
  SDL_Window *window = SDL_CreateWindow(spec.title.c_str(), spec.width,
                                        spec.height, SDL_WINDOW_RESIZABLE);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Uint64 last = SDL_GetPerformanceCounter();
  const double freq = (double)SDL_GetPerformanceFrequency();

  bool running = true;
  while (running && !game.WantsQuit()) {
    Uint64 now = SDL_GetPerformanceCounter();
    double dt = (double)(now - last) / freq;
    last = now;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        running = false;
      else
        game.OnEvent(e);
    }

    int w = 0, h = 0;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    game.Resize(w, h);

    game.Update(dt);

    // o Game só sinaliza; quem mexe na janela é o App
    if (auto change = game.TakeWindowChange()) {
      SDL_SetWindowTitle(window, change->title.c_str());
      SDL_SetWindowSize(window, change->width, change->height);
    }

    game.Draw(renderer);
    SDL_RenderPresent(renderer);
  }

  game.Unload(); // para a thread de física antes de fechar o SDL
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

} // namespace galileu
