#include "GameScene.h"
#include "core/Logger.h"

namespace game {

GameScene::GameScene(engine::ecs::Registry& registry, engine::Window& window,
                      engine::InputManager& input, std::function<void()> onQuit)
    : registry_(registry), window_(window), input_(input), onQuit_(std::move(onQuit)) {
}

void GameScene::onEnter() {
    LOG_INFO(engine::LogCategory::Core, "GameScene entered");
    // TODO: Initialize game world, create entities, etc.
    
    // Initialize audio manager (stub for now)
    audioManager_ = std::make_unique<engine::audio::AudioManager>();
    audioManager_->setInitialized(true);
    
    pauseMenu_ = std::make_unique<game::ui::PauseMenu>(
        window_.renderer(), window_.width(), window_.height(),
        "assets/fonts/Inter-Regular.ttf", onQuit_, &window_, audioManager_.get());
}

void GameScene::onExit() {
    LOG_INFO(engine::LogCategory::Core, "GameScene exited");
    // TODO: Clean up game world
    pauseMenu_.reset();
}

void GameScene::update(double dt) {
    // TODO: Update game logic
    
    // engine::Action::Pause (Escape) -- Application.cpp has carried a
    // placeholder for this exact hookup since Milestone 1:
    //   "Pause-as-quit is a Milestone-1 placeholder; a real pause menu
    //    will intercept this once the UI system exists."
    // Application.cpp's own placeholder block never actually did
    // anything (its body was just that comment), so there's no
    // conflicting behavior to remove there -- this is a clean, purely
    // additive interception via GameScene's own input polling.
    if (pauseMenu_ && input_.wasPressed(engine::Action::Pause)) {
        LOG_INFO(engine::LogCategory::Core, "ESC pressed, current pause menu state: {}", pauseMenu_->isOpen());
        if (pauseMenu_->isOpen()) {
            pauseMenu_->setOpen(false);  // Close if open
        } else {
            pauseMenu_->setOpen(true);   // Open if closed
        }
    }
    
    if (pauseMenu_) {
        pauseMenu_->handleInput(input_);
        pauseMenu_->update(dt);
    }
}

void GameScene::render() {
    // TODO: Render game
    
    if (pauseMenu_) {
        pauseMenu_->render();
    }
}

} // namespace game
