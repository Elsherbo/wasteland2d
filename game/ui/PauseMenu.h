#pragma once

#include <SDL.h>
#include <functional>
#include <memory>
#include <string>

#include "audio/AudioManager.h"
#include "core/Logger.h"
#include "core/Window.h"
#include "input/InputManager.h"
#include "ui/UIComponent.h"
#include "ui/UIManager.h"
#include "ui/UIRenderer.h"
#include "ui/UIStyle.h"
#include "render/Font.h"
#include "render/TextRenderer.h"

namespace game::ui {

// The pause/settings menu, toggled by engine::Action::Pause (Escape),
// which Application.cpp has had a placeholder for since Milestone 1:
//   "Pause-as-quit is a Milestone-1 placeholder; a real pause menu
//    will intercept this once the UI system exists."
// This is that menu. It's also the first non-demo caller of
// engine::ui -- deliberately built from the exact same composition
// pattern proven out (and repeatedly fixed) in
// tests/ui_professional_demo.cpp: a ScrollContainer ("Panel") wrapping
// an autoSize VBox of Section cards, rather than inventing a new shape.
//
// Nothing in here is wired to real graphics/audio systems yet --
// neither exists in the codebase as of this writing (no AudioManager,
// no persisted settings). The checkboxes/sliders are real, interactive,
// and visually correct; their onChanged callbacks currently only log,
// as scaffolding for whenever those systems exist.
class PauseMenu {
public:
    // fontPath: a real bundled asset (assets/fonts/Inter-Regular.ttf),
    // not the demos' hardcoded Windows system font -- this needs to run
    // on whatever machine actually ships the game.
    // onQuit: invoked when the player picks "Quit to Desktop".
    PauseMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight,
              const std::string& fontPath, std::function<void()> onQuit,
              engine::Window* window, engine::audio::AudioManager* audio);
    ~PauseMenu() = default;
    
    PauseMenu(const PauseMenu&) = delete;
    PauseMenu& operator=(const PauseMenu&) = delete;
    
    bool isOpen() const { return open_; }
    void setOpen(bool open) { 
        LOG_INFO(engine::LogCategory::Core, "PauseMenu setOpen: {} -> {}", open_, open);
        open_ = open; 
    }
    void toggle() { open_ = !open_; }
    
    // Polls InputManager's current mouse state (position/buttons/wheel)
    // and feeds it into this menu's own UIManager, the same shape both
    // UI demos feed from raw SDL events -- just pull-based here since
    // GameScene has no access to the raw SDL event stream (Application
    // owns that loop; Scene only gets update()/render()). No-ops
    // entirely while closed.
    void handleInput(engine::InputManager& input);
    
    // Drives layout (auto-sizing, scroll clamping, etc.) -- no-ops
    // while closed.
    void update(double dt);
    
    // No-ops while closed.
    void render();

private:
    void buildUI(int screenWidth, int screenHeight);
    
    std::function<void()> onQuit_;
    engine::Window* window_;           // For VSync and fullscreen control
    engine::audio::AudioManager* audio_; // For volume control
    bool open_ = false;
    
    engine::render::Font font_;
    engine::render::Font headingFont_;
    engine::render::TextRenderer textRenderer_;
    engine::ui::UIManager uiManager_;
    engine::ui::UIRenderer uiRenderer_;
    
    // Root of the built tree (the outer "Panel" ScrollContainer).
    std::unique_ptr<engine::ui::UIComponent> root_;
    
    // InputManager only exposes absolute mouse position
    // (getMousePosition()), unlike the SDL_MOUSEMOTION event both demos
    // read xrel/yrel from directly -- this is what lets handleInput()
    // compute the relative motion InputEvent::mouseMotion() wants.
    int lastMouseX_ = 0;
    int lastMouseY_ = 0;
};

} // namespace game::ui
