#pragma once

#include "core/Application.h"
#include "core/EventBus.h"
#include "core/Logger.h"
#include "core/ServiceLocator.h"
#include "scenes/SceneManager.h"
#include "systems/SystemRegistry.h"
#include "resources/ResourceManager.h"
#include "entities/EntityFactory.h"
#include "ecs/Registry.h"
#include <memory>

namespace game {

// Main game class that coordinates all systems
class Game {
public:
    Game(const engine::ApplicationConfig& config);
    ~Game();
    
    // Delete copy operations
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    
    // Run the game loop
    void run();
    
    // Stop the game
    void stop();
    
    // Accessors
    engine::ecs::Registry& registry() { return registry_; }
    engine::SceneManager& sceneManager() { return sceneManager_; }
    engine::SystemRegistry& systemRegistry() { return systemRegistry_; }
    engine::EventBus& eventBus() { return eventBus_; }
    engine::ResourceManager& resourceManager() { return resourceManager_; }
    engine::entities::EntityFactory& entityFactory() { return entityFactory_; }
    engine::Application& application() { return *application_; }

private:
    void initialize();
    void shutdown();
    
    // Core systems
    std::unique_ptr<engine::Application> application_;
    engine::ecs::Registry registry_;
    engine::SceneManager sceneManager_;
    engine::SystemRegistry systemRegistry_;
    engine::EventBus eventBus_;
    engine::ResourceManager resourceManager_;
    engine::entities::EntityFactory entityFactory_;
    
    bool running_;
};

} // namespace game
