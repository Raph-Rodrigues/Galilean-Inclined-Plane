#include "script_host.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <algorithm>
#include <cstdio>
#include <utility>

namespace galileu {
namespace {

// Os helpers leem um campo da tabela que está no TOPO da pilha.
double Num(lua_State *L, const char *k, double def) {
  lua_getfield(L, -1, k);
  double v = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : def;
  lua_pop(L, 1);
  return v;
}

std::string Str(lua_State *L, const char *k, const std::string &def) {
  lua_getfield(L, -1, k);
  std::string v = lua_isstring(L, -1) ? lua_tostring(L, -1) : def;
  lua_pop(L, 1);
  return v;
}

void Color(lua_State *L, const char *k, ColorRgb &out) {
  lua_getfield(L, -1, k);
  if (lua_istable(L, -1)) {
    float c[3];
    for (int i = 0; i < 3; ++i) {
      lua_rawgeti(L, -1, i + 1);
      c[i] = (float)lua_tonumber(L, -1) / 255.0f;
      lua_pop(L, 1);
    }
    out = ColorRgb{c[0], c[1], c[2]};
  }
  lua_pop(L, 1);
}

// Chama fn(L) com a subtabela `name` no topo da pilha (se existir).
template <typename F> void Section(lua_State *L, const char *name, F fn) {
  lua_getfield(L, -1, name);
  if (lua_istable(L, -1))
    fn(L);
  lua_pop(L, 1);
}

} // namespace

ScriptHost::ScriptHost() {
  L_ = luaL_newstate();
  luaL_openlibs(L_);
}

ScriptHost::~ScriptHost() {
  if (L_)
    lua_close(L_);
}

bool ScriptHost::LoadConfig(const std::string &path, Config &out,
                            std::string *err) {
  lua_State *L = L_;
  int top = lua_gettop(L);

  if (luaL_dofile(L, path.c_str()) != LUA_OK) {
    const char *m = lua_tostring(L, -1);
    std::string msg = m ? m : "erro desconhecido";
    fprintf(stderr, "[LUA] erro: %s\n", msg.c_str());
    if (err)
      *err = msg;
    lua_settop(L, top);
    return false;
  }
  if (lua_gettop(L) != top + 1 || !lua_istable(L, -1)) {
    fprintf(stderr, "[LUA] %s deve retornar uma tabela\n", path.c_str());
    if (err)
      *err = "o script deve retornar uma tabela";
    lua_settop(L, top);
    return false;
  }

  Config c; // padrões: campos removidos do script voltam ao valor padrão
  c.title = Str(L, "title", c.title);
  c.width = (int)Num(L, "width", c.width);
  c.height = (int)Num(L, "height", c.height);

  Section(L, "physics", [&](lua_State *L) {
    c.g = Num(L, "g", c.g);
    c.h0 = Num(L, "h0", c.h0);
    c.theta1 = Num(L, "theta1", c.theta1);
    c.theta2 = Num(L, "theta2", c.theta2);
  });
  Section(L, "ball", [&](lua_State *L) {
    c.radius = Num(L, "radius", c.radius);
    Color(L, "color", c.ballColor);
  });
  Section(L, "view", [&](lua_State *L) {
    c.scale = Num(L, "scale", c.scale);
    c.velScale = Num(L, "vel_scale", c.velScale);
    c.maxRun = Num(L, "max_run", c.maxRun);
  });

  lua_getfield(L, -1, "angles");
  if (lua_istable(L, -1)) {
    int n = (int)lua_rawlen(L, -1);
    for (int i = 1; i <= n; ++i) {
      lua_rawgeti(L, -1, i);
      if (lua_isnumber(L, -1))
        c.angles.push_back(lua_tonumber(L, -1));
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);
  lua_settop(L, top); // remove a tabela retornada

  if (c.g <= 0 || c.h0 <= 0 || c.radius <= 0 || c.scale <= 0 || c.width < 200 ||
      c.height < 200) {
    fprintf(stderr, "[LUA] valores invalidos em %s\n", path.c_str());
    if (err)
      *err = "valores invalidos (g, h0, radius, scale devem ser > 0)";
    return false;
  }
  c.theta1 = std::clamp(c.theta1, 1.0, 85.0);
  c.theta2 = std::clamp(c.theta2, 0.0, 85.0);

  out = std::move(c);
  return true;
}

} // namespace galileu
