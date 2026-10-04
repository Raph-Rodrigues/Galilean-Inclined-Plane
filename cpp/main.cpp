#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cstdio>
#include <string>

extern "C" double physics_add(double a, double b); // rust

int main(int argc, char *argv[]) {
  // rust
  printf("[Rust] 2 + 3 = %.1f\n", physics_add(2.0, 3.0));

  // lua
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  if (luaL_dofile(L, GALILEU_SCRIPTS_DIR "/config.lua") != LUA_OK) {
    fprintf(stderr, "[LUA] erro: %s\n", lua_tostring(L, -1));
    return 1;
  }
  // a tabela retornada esta no topo da pilha
  auto get_str = [&](const char *k) {
    lua_getfield(L, -1, k);
    std::string s = lua_tostring(L, -1);
    lua_pop(L, 1);
    return s;
  };

  auto get_int = [&](const char *k) {
    lua_getfield(L, -1, k);
    int v = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return v;
  };

  std::string title = get_str("title");
  int w = get_int("width");
  int h = get_int("height");
  lua_pop(L, 1);
  printf("[LUA] %s (%dx%d)\n", title.c_str(), w, h);

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_init: %s\n", SDL_GetError());
    return -1;
  }

  SDL_Window *window =
      SDL_CreateWindow(title.c_str(), w, h, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

  if (!window) {
    fprintf(stderr, "SDL_window: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  if (!renderer) {
    fprintf(stderr, "SDL_renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  bool running = true;
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
        running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 10, 10, "C++ + Rust + lua: tudo conectado!");
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  lua_close(L);
  return 0;
}
