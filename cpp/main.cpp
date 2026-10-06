#include "physics.hpp"
#include <SDL3/SDL.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

constexpr double deg2rad(double d) { return d * std::numbers::pi / 180.0; }

struct Vec2 {
  double x, y;
};

// ---------------- Câmera: metros (y p/ cima) -> pixels (y p/ baixo) -------
struct Camera {
  double scale = 40.0;   // pixels por metro
  Vec2 center{0.0, 2.5}; // ponto do mundo no centro da tela
  int w = 1280, h = 720;

  SDL_FPoint toScreen(Vec2 p) const {
    return {(float)(w * 0.5 + (p.x - center.x) * scale),
            (float)(h * 0.5 - (p.y - center.y) * scale)};
  }
};

// ---------------- Helpers de desenho --------------------------------------
static void draw_line(SDL_Renderer *r, const Camera &cam, Vec2 a, Vec2 b) {
  SDL_FPoint pa = cam.toScreen(a), pb = cam.toScreen(b);
  SDL_RenderLine(r, pa.x, pa.y, pb.x, pb.y);
}

static void draw_dashed(SDL_Renderer *r, const Camera &cam, Vec2 a, Vec2 b,
                        double dash) {
  double dx = b.x - a.x, dy = b.y - a.y;
  double len = std::sqrt(dx * dx + dy * dy);
  int n = (int)(len / dash);
  for (int i = 0; i < n; i += 2) {
    double t0 = (double)i / n, t1 = (double)(i + 1) / n;
    draw_line(r, cam, {a.x + dx * t0, a.y + dy * t0},
              {a.x + dx * t1, a.y + dy * t1});
  }
}

static void fill_circle(SDL_Renderer *r, const Camera &cam, Vec2 c,
                        double radius, SDL_FColor color) {
  const int seg = 32;
  SDL_FPoint center = cam.toScreen(c);
  float rp = (float)(radius * cam.scale);
  std::vector<SDL_Vertex> v;
  v.reserve(seg * 3);
  for (int i = 0; i < seg; ++i) {
    float a0 = 2.0f * (float)std::numbers::pi * i / seg;
    float a1 = 2.0f * (float)std::numbers::pi * (i + 1) / seg;
    SDL_Vertex c0{center, color, {0, 0}};
    SDL_Vertex p0{{center.x + rp * std::cos(a0), center.y + rp * std::sin(a0)},
                  color,
                  {0, 0}};
    SDL_Vertex p1{{center.x + rp * std::cos(a1), center.y + rp * std::sin(a1)},
                  color,
                  {0, 0}};
    v.push_back(c0);
    v.push_back(p0);
    v.push_back(p1);
  }
  SDL_RenderGeometry(r, nullptr, v.data(), (int)v.size(), nullptr, 0);
}

// seta em coordenadas de tela, duas "barbas" na ponta
static void draw_arrow_px(SDL_Renderer *r, SDL_FPoint a, SDL_FPoint b) {
  SDL_RenderLine(r, a.x, a.y, b.x, b.y);
  float dx = b.x - a.x, dy = b.y - a.y;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 6.0f)
    return; // curta de mais para ter uma ponta
  float ux = dx / len, uy = dy / len;
  const float head = 10.0f, c = 0.9063f, s = 0.4226f; // cos/sen de 25°
  SDL_RenderLine(r, b.x, b.y, b.x - head * (ux * c - uy * s),
                 b.y - head * (ux * s + uy * c));
  SDL_RenderLine(r, b.x, b.y, b.x - head * (ux * c + uy * s),
                 b.y - head * (uy * c - ux * s));
}

static void draw_arrow(SDL_Renderer *r, const Camera &cam, Vec2 from, Vec2 to) {
  draw_arrow_px(r, cam.toScreen(from), cam.toScreen(to));
}

static void draw_text(SDL_Renderer *r, const Camera &cam, Vec2 p, float dx,
                      float dy, const char *text) {
  SDL_FPoint s = cam.toScreen(p);
  SDL_RenderDebugText(r, s.x + dx, s.y + dy, text);
}

int main(int argc, char *argv[]) {
  // ---------- Lua ----------
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  if (luaL_dofile(L, GALILEU_SCRIPTS_DIR "/config.lua") != LUA_OK) {
    fprintf(stderr, "[LUA] erro: %s\n", lua_tostring(L, -1));
    return 1;
  }
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

  // ---------- SDL3 ----------
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_init: %s\n", SDL_GetError());
    return -1;
  }
  SDL_Window *window =
      SDL_CreateWindow(title.c_str(), w, h, SDL_WINDOW_RESIZABLE);
  if (!window) {
    fprintf(stderr, "SDL_window: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    fprintf(stderr, "SDL_renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // ---------- Cena ----------
  const double H = 5.0;
  const double radius = 0.3;
  const double theta1 = deg2rad(45.0);
  double theta2_deg = 30.0;
  const double maxRun = 15.0;
  const double g = 9.81;

  PhysicsWorld *world = physics_create(g, H, theta1, deg2rad(theta2_deg));
  if (!world) {
    fprintf(stderr, "physics_create falhou\n");
    return -1;
  }

  Camera cam;

  // ---------- Loop de passo fixo ----------
  const double dt = 1.0 / 120.0;
  double accumulator = 0.0;
  double simTime = 0.0;
  long steps = 0;
  Uint64 last = SDL_GetPerformanceCounter();
  const double freq = (double)SDL_GetPerformanceFrequency();

  bool running = true;
  while (running) {
    Uint64 now = SDL_GetPerformanceCounter();
    double frameTime = std::min((double)(now - last) / freq, 0.25);
    last = now;
    accumulator += frameTime;

    // --- entrada ---
    bool restart = false;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        running = false;
      if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_ESCAPE)
          running = false;
        if (e.key.key == SDLK_LEFT || e.key.key == SDLK_RIGHT) {
          theta2_deg += (e.key.key == SDLK_RIGHT) ? -5.0 : 5.0;
          theta2_deg = std::clamp(theta2_deg, 0.0, 75.0);
          physics_reset(world, g, H, theta1, deg2rad(theta2_deg));
          simTime = 0.0;
        }
        if (e.key.key == SDLK_R) {
          physics_reset(world, g, H, theta1, deg2rad(theta2_deg));
          simTime = 0.0;
        }
      }
    }

    // --- atualização: passos fixos ---
    while (accumulator >= dt) {
      // (Etapa 2: aqui chamaremos o passo de física em Rust)
      physics_step(world, dt);
      simTime += dt;
      ++steps;
      accumulator -= dt;
    }

    // --- tamanho atual da janela ---
    SDL_GetRenderOutputSize(renderer, &cam.w, &cam.h);

    // --- geometria dos planos ---
    const double theta2 = deg2rad(theta2_deg);
    Vec2 valley{0.0, 0.0};
    Vec2 leftTop{-H / std::sin(theta1) * std::cos(theta1), H};
    double run2 = (theta2 > 1e-6) ? H / std::sin(theta2) : 1e9;
    run2 = std::min(run2, maxRun / std::max(std::cos(theta2), 1e-6));
    Vec2 rightEnd{run2 * std::cos(theta2), run2 * std::sin(theta2)};

    // bola: sobre o plano esquerdo, na altura H (centro deslocado pela normal)
    PhysicsState st;
    physics_get_state(world, &st);
    Vec2 n{-st.ty, st.tx}; // normal do plano (aponta "para cima")
    Vec2 ball{st.x + radius * n.x, st.y + radius * n.y};

    // --- desenho ---
    char buf[160];
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // linha de altura de referência
    SDL_SetRenderDrawColor(renderer, 120, 120, 140, 255);
    draw_dashed(renderer, cam, {-12.0, H}, {maxRun, H}, 0.4);
    snprintf(buf, sizeof buf, "altura inicial h0 = %.1f m", H);
    draw_text(renderer, cam, {-12.0, H}, 4, -14, buf);

    // planos + legendas
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    draw_line(renderer, cam, leftTop, valley);
    draw_line(renderer, cam, valley, rightEnd);
    snprintf(buf, sizeof buf, "Plano 1: %.1f graus", 45.0);
    draw_text(renderer, cam, {leftTop.x * 0.5, leftTop.y * 0.5}, -140, 6, buf);
    snprintf(buf, sizeof buf, "Plano 2: %.1f graus", theta2_deg);
    draw_text(renderer, cam, {rightEnd.x * 0.5, rightEnd.y * 0.5}, -40, 16,
              buf);

    // bola
    fill_circle(renderer, cam, ball, radius,
                SDL_FColor{1.0f, 0.6f, 0.2f, 1.0f});

    // vetor velocidade (v com sinal * tangente) + módulo ao lado da seta
    const double velScale = 0.5; // metros de seta por (m/s)
    Vec2 vvec{st.v * st.tx, st.v * st.ty};
    Vec2 tip{ball.x + vvec.x * velScale, ball.y + vvec.y * velScale};
    SDL_SetRenderDrawColor(renderer, 90, 230, 130, 255);
    draw_arrow(renderer, cam, ball, tip);
    snprintf(buf, sizeof buf, "v = %.2f m/s", std::fabs(st.v));
    draw_text(renderer, cam, tip, 8, -14, buf);

    // módulo da velocidade em destaque (texto 2x)
    SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    snprintf(buf, sizeof buf, "|v| = %.2f m/s", std::fabs(st.v));
    SDL_RenderDebugText(renderer, 10.0f / 2.0f, (cam.h - 70.0f) / 2.0f, buf);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    // descrição da situação atual
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if (theta2_deg < 0.5) {
      SDL_RenderDebugText(renderer, 10, cam.h - 40.0f,
                          "Plano 2 horizontal: sem forca ao longo do movimento "
                          "-> MRU (inercia)");
    } else {
      snprintf(buf, sizeof buf,
               "Plano 2 a %.1f graus: a bola sobe ate a altura inicial h0",
               theta2_deg);
      SDL_RenderDebugText(renderer, 10, cam.h - 40.0f, buf);
    }

    // caixa de legenda (canto superior direito)
    {
      float lx = cam.w - 250.0f, ly = 12.0f;
      SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
      SDL_RenderLine(renderer, lx, ly + 4, lx + 24, ly + 4);
      SDL_RenderDebugText(renderer, lx + 32, ly, "plano inclinado");

      ly += 16;
      SDL_SetRenderDrawColor(renderer, 120, 120, 140, 255);
      SDL_RenderLine(renderer, lx, ly + 4, lx + 10, ly + 4);
      SDL_RenderLine(renderer, lx + 16, ly + 4, lx + 24, ly + 4);
      SDL_RenderDebugText(renderer, lx + 32, ly, "altura inicial h0");

      ly += 16;
      SDL_SetRenderDrawColor(renderer, 255, 153, 51, 255);
      SDL_FRect sw{lx + 6, ly, 12, 8};
      SDL_RenderFillRect(renderer, &sw);
      SDL_RenderDebugText(renderer, lx + 32, ly, "bola");

      ly += 16;
      SDL_SetRenderDrawColor(renderer, 90, 230, 130, 255);
      draw_arrow_px(renderer, {lx, ly + 4}, {lx + 24, ly + 4});
      SDL_RenderDebugText(renderer, lx + 32, ly, "vetor velocidade");
    }

    // HUD numérico
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    snprintf(buf, sizeof buf, "theta1 = 45.0 deg   theta2 = %.1f deg",
             theta2_deg);
    SDL_RenderDebugText(renderer, 10, 10, buf);
    snprintf(buf, sizeof buf, "t = %.2f s   passos = %ld   dt = 1/120 s",
             simTime, steps);
    SDL_RenderDebugText(renderer, 10, 24, buf);
    snprintf(buf, sizeof buf, "s = %7.3f m   v = %7.3f m/s   h = %6.3f m", st.s,
             st.v, st.height);
    SDL_RenderDebugText(renderer, 10, 38, buf);
    snprintf(buf, sizeof buf, "E/m = %.6f J/kg", st.energy);
    SDL_RenderDebugText(renderer, 10, 52, buf);
    SDL_RenderDebugText(renderer, 10, 66,
                        "Setas: muda theta2 | R: reinicia | ESC: sair");

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  physics_destroy(world);
  lua_close(L);
  return 0;
}
