#pragma once

namespace galileu {

// Dono do SDL: janela, renderer, loop principal, tempo e eventos.
// Não sabe nada de física nem de Lua: só conversa com o Game.
class App {
public:
  int Run();
};

} // namespace galileu
