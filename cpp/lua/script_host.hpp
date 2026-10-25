#pragma once
#include <string>

#include "config.hpp"

struct lua_State;

namespace galileu {

// Ponte com o Lua. É o ÚNICO módulo que enxerga a API C do Lua.
// O lua_State não é thread-safe: só a thread principal usa este objeto.
class ScriptHost {
public:
  ScriptHost();
  ~ScriptHost();
  ScriptHost(const ScriptHost &) = delete;
  ScriptHost &operator=(const ScriptHost &) = delete;

  // Executa o script a partir de uma Config limpa (padrões). Só sobrescreve
  // `out` se tudo der certo; em erro, devolve a mensagem em *err.
  bool LoadConfig(const std::string &path, Config &out,
                  std::string *err = nullptr);

private:
  lua_State *L_ = nullptr;
};

} // namespace galileu
