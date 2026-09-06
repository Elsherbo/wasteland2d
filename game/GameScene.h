#pragma once

#include "scenes/Scene.h"
#include "ecs/Registry.h"
#include "core/Window.h"
#include "input/InputManager.h"
#include "audio/AudioManager.h"
#include "ui/PauseMenu.h"
#include <functional>
#include <memory>

namespace game {

// Game scene that contains the actual game logic
class GameScene : public engine::Scene {
public:
    // window/input: needed to construct and drive PauseMenu (a real
    // SDL_Renderer* to build a UIRenderer against, and polled mouse
    // state -- GameScene has no access to the raw SDL event stream,
    // Application owns that loop).
    // onQuit: forwarded to PauseMenu's "Quit to Desktop" button.
    GameScene(engine::ecs::Registry& registry, engine::Window& window,
              engine::InputManager& input, std::function<void()> onQuit);
    ~GameScene() override = default;
    
    void onEnter() override;
    void onExit() override;
    void update(double dt) override;
    void render() override;
    const char* getName() const override { return "GameScene"; }

private:
    engine::ecs::Registry& registry_;
    engine::Window& window_;
    engine::InputManager& input_;
    std::function<void()> onQuit_;
    
    // Audio manager for volume control
    std::unique_ptr<engine::audio::AudioManager> audioManager_;
    
    // Built in onEnter() (once window_.renderer() is guaranteed valid)
    // and torn down in onExit(), matching this class's existing
    // enter/exit lifecycle convention rather than the constructor.
    std::unique_ptr<game::ui::PauseMenu> pauseMenu_;
};

} // namespace game
