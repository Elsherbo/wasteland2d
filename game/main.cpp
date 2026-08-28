#include "Game.h"
#include "core/Logger.h"

int main(int, char**) {
    // Configure application
    engine::ApplicationConfig config;
    config.title = "wasteland2d - Professional Architecture";
    config.width = 1280;
    config.height = 720;
    config.fixedUpdateHz = 60.0;
    
    // Configure logger
    config.loggerConfig.level = engine::LogLevel::Info;
    config.loggerConfig.enableColors = true;
    config.loggerConfig.enableFileOutput = false;
    config.loggerConfig.enableTimestamps = true;
    config.loggerConfig.enableCategories = true;

    // Create and run game
    game::Game game(config);
    game.run();
    
    return 0;
}
