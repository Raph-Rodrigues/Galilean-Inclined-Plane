#pragma once
#include <SDL3/SDL.h>

#include <filesystem>
#include <optional>
#include <string>

#include "core/history.hpp"
#include "game/sequence.hpp"
#include "lua/config.hpp"
#include "lua/script_host.hpp"
#include "physics/simulation.hpp"
#include "render/camera.hpp"
#include "render/hud.hpp"
#include "render/scene_model.hpp"

namespace galileu {

struct WindowSpec {
  std::string title;
  int width, height;
};

// O "jogo": ciclo de vida estilo engine.
//   Load()    -> carrega configuração e cria a simulação
//   OnEvent() -> entrada
//   Update()  -> lógica (lê o Rust, avança a Sequence, monta o SceneModel,
//   câmera) Draw()    -> desenha (só lê dados) Unload()  -> libera tudo
class Game {
public:
  ~Game() { Unload(); }

  bool Load();
  void OnEvent(const SDL_Event &e);
  void Resize(int w, int h);
  void Update(double dt);
  void Draw(SDL_Renderer *r);
  void Unload();

  bool WantsQuit() const { return quit_; }
  WindowSpec Window() const;
  std::optional<WindowSpec> TakeWindowChange(); // preenchido após hot reload

private:
  void BeginSituation(size_t index);
  void ReloadConfig();
  void PollHotReload(double dt);
  void BuildModel(const SimSnapshot &snap);
  void UpdateCamera(double dt);

  ScriptHost script_;
  Config cfg_;
  Simulation sim_;
  Sequence seq_;
  Camera cam_;
  SceneModel model_;
  History history_;
  ViewOptions opts_;
  StatusMessage status_;

  std::filesystem::file_time_type cfgStamp_{};
  double pollTimer_ = 0.0;
  double fps_ = 0.0;
  int speedIdx_ = 3; // índice em kSpeeds (x1)
  bool paused_ = false;
  bool camInit_ = false;
  bool quit_ = false;
  bool windowDirty_ = false;
};

} // namespace galileu
