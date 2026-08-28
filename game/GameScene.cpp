#include "GameScene.h"
#include "core/Logger.h"

namespace game {

GameScene::GameScene(engine::ecs::Registry& registry)
    : registry_(registry) {
}

void GameScene::onEnter() {
    LOG_INFO(engine::LogCategory::Core, "GameScene entered");
    // TODO: Initialize game world, create entities, etc.
}

void GameScene::onExit() {
    LOG_INFO(engine::LogCategory::Core, "GameScene exited");
    // TODO: Clean up game world
}

void GameScene::update(double dt) {
    // TODO: Update game logic
    (void)dt; // Suppress unused warning
}

void GameScene::render() {
    // TODO: Render game
}

} // namespace game
