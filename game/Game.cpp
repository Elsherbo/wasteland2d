#include "Game.h"
#include "GameScene.h"
#include "core/Logger.h"

namespace game {

Game::Game(const engine::ApplicationConfig& config)
    : entityFactory_(registry_), running_(false) {
    
    // Create application
    application_ = std::make_unique<engine::Application>(config);
    
    // Initialize core systems
    initialize();
}

Game::~Game() {
    shutdown();
}

void Game::initialize() {
    LOG_INFO(engine::LogCategory::Core, "Initializing Game");
    
    // Register services with Service Locator
    engine::ServiceLocator::provide<engine::EventBus>(&eventBus_);
    engine::ServiceLocator::provide<engine::ResourceManager>(&resourceManager_);
    engine::ServiceLocator::provide<engine::SceneManager>(&sceneManager_);
    engine::ServiceLocator::provide<engine::SystemRegistry>(&systemRegistry_);
    engine::ServiceLocator::provide<engine::entities::EntityFactory>(&entityFactory_);
    
    // Load game scene
    auto gameScene = std::make_unique<GameScene>(registry_);
    sceneManager_.loadScene(std::move(gameScene));
    
    // Setup application callbacks
    application_->setUpdateCallback([this](double dt) {
        sceneManager_.update(dt);
        systemRegistry_.updateAll(dt);
    });
    
    application_->setRenderCallback([this](engine::Window& window, double alpha) {
        (void)window; // Suppress unused warning
        (void)alpha; // Suppress unused warning
        sceneManager_.render();
        systemRegistry_.renderAll();
    });
    
    LOG_INFO(engine::LogCategory::Core, "Game initialized successfully");
}

void Game::shutdown() {
    LOG_INFO(engine::LogCategory::Core, "Shutting down Game");
    
    // Clear systems
    systemRegistry_.shutdownAll();
    systemRegistry_.clear();
    sceneManager_.clear();
    resourceManager_.clear();
    
    // Clear service locator
    engine::ServiceLocator::clear();
    
    LOG_INFO(engine::LogCategory::Core, "Game shutdown complete");
}

void Game::run() {
    LOG_INFO(engine::LogCategory::Core, "Starting game loop");
    running_ = true;
    
    // Run the application
    application_->run();
    
    running_ = false;
    LOG_INFO(engine::LogCategory::Core, "Game loop ended");
}

void Game::stop() {
    LOG_INFO(engine::LogCategory::Core, "Stopping game");
    running_ = false;
    application_->quit();
}

} // namespace game
