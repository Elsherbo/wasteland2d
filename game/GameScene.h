#pragma once

#include "scenes/Scene.h"
#include "ecs/Registry.h"
#include <memory>

namespace game {

// Game scene that contains the actual game logic
class GameScene : public engine::Scene {
public:
    explicit GameScene(engine::ecs::Registry& registry);
    ~GameScene() override = default;
    
    void onEnter() override;
    void onExit() override;
    void update(double dt) override;
    void render() override;
    const char* getName() const override { return "GameScene"; }

private:
    engine::ecs::Registry& registry_;
};

} // namespace game
