// Visual test for UI system
#include <SDL.h>
#include <SDL_ttf.h>
#include <glm/vec2.hpp>
#include <functional>
#include <iostream>
#include <memory>

#include "ui/UIComponent.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UISlider.h"
#include "ui/UICheckbox.h"
#include "ui/UIVBox.h"
#include "ui/UIHBox.h"
#include "ui/UIGrid.h"
#include "ui/UIRenderer.h"
#include "ui/UIManager.h"
#include "ui/InputEvent.h"
#include "render/Font.h"
#include "render/TextRenderer.h"
#include "render/Color.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "UI System Visual Test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Load font
    engine::render::Font font("C:\\Windows\\Fonts\\arial.ttf", 16);
    engine::render::TextRenderer textRenderer(renderer);
    
    // Create UI renderer
    engine::ui::UIRenderer uiRenderer(renderer, textRenderer, font);
    
    // Create UI manager
    engine::ui::UIManager uiManager;
    
    // Get UI layer
    engine::ui::UILayer* uiLayer = uiManager.getLayer("UI");
    
    // Create UI hierarchy
    auto vbox = std::make_unique<engine::ui::UIVBox>();
    vbox->setPosition(glm::vec2(50, 50));
    vbox->setSize(glm::vec2(300, 400));
    vbox->setSpacing(10);
    vbox->setPadding(glm::vec2(10));
    
    // Add label
    auto label = std::make_unique<engine::ui::UILabel>();
    label->setText("UI System Test");
    label->setSize(glm::vec2(280, 30));
    vbox->addChild(std::move(label));
    
    // Add button
    auto button = std::make_unique<engine::ui::UIButton>();
    button->setText("Click Me");
    button->setSize(glm::vec2(280, 40));
    button->onClick = []() {
        std::cout << "Button clicked!" << std::endl;
    };
    vbox->addChild(std::move(button));
    
    // Add checkbox
    auto checkbox = std::make_unique<engine::ui::UICheckbox>();
    checkbox->setText("Enable Feature");
    checkbox->setSize(glm::vec2(280, 30));
    checkbox->onCheckedChanged = [](bool checked) {
        std::cout << "Checkbox: " << (checked ? "checked" : "unchecked") << std::endl;
    };
    vbox->addChild(std::move(checkbox));
    
    // Add slider
    auto slider = std::make_unique<engine::ui::UISlider>();
    slider->setSize(glm::vec2(280, 20));
    slider->setValue(0.5f);
    slider->onValueChanged = [](float value) {
        std::cout << "Slider value: " << value << std::endl;
    };
    vbox->addChild(std::move(slider));
    
    // Add image
    auto image = std::make_unique<engine::ui::UIImage>();
    image->setSize(glm::vec2(280, 50));
    image->setTintColor(glm::vec4(0.2f, 0.5f, 0.8f, 1.0f));
    vbox->addChild(std::move(image));
    
    // Layout the container
    vbox->layout();
    
    // Add to UI layer
    uiLayer->addComponent(vbox.get());
    
    // Keep ownership in a container
    std::vector<std::unique_ptr<engine::ui::UIComponent>> components;
    components.push_back(std::move(vbox));
    
    // Main loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    running = false;
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                // Convert SDL event to InputEvent
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseMotion(
                    static_cast<float>(event.motion.x),
                    static_cast<float>(event.motion.y),
                    static_cast<float>(event.motion.xrel),
                    static_cast<float>(event.motion.yrel)
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Convert SDL event to InputEvent
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseButton(
                    static_cast<float>(event.button.x),
                    static_cast<float>(event.button.y),
                    event.button.button,
                    true
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                // Convert SDL event to InputEvent
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseButton(
                    static_cast<float>(event.button.x),
                    static_cast<float>(event.button.y),
                    event.button.button,
                    false
                );
                uiManager.dispatchInput(inputEvent);
            }
        }
        
        // Update UI
        uiManager.update(0.016);  // ~60 FPS
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
        SDL_RenderClear(renderer);
        
        // Render UI directly using UIRenderer
        uiRenderer.render(components[0].get());
        
        // Present
        SDL_RenderPresent(renderer);
        
        // Cap framerate
        SDL_Delay(16);
    }
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
