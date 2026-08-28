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
    vbox->addChild(std::move(button));
    
    // Add checkbox
    auto checkbox = std::make_unique<engine::ui::UICheckbox>();
    checkbox->setText("Enable Feature");
    checkbox->setSize(glm::vec2(280, 30));
    vbox->addChild(std::move(checkbox));
    
    // Add slider
    auto slider = std::make_unique<engine::ui::UISlider>();
    slider->setSize(glm::vec2(280, 20));
    slider->setValue(0.5f);
    vbox->addChild(std::move(slider));
    
    // Add image
    auto image = std::make_unique<engine::ui::UIImage>();
    image->setSize(glm::vec2(280, 50));
    image->setTintColor(glm::vec4(0.2f, 0.5f, 0.8f, 1.0f));
    vbox->addChild(std::move(image));
    
    // Layout the container
    vbox->layout();
    
    // Get raw pointer for rendering
    engine::ui::UIComponent* root = vbox.get();
    
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
                // Update hover state for all components
                int mouseX = event.motion.x;
                int mouseY = event.motion.y;
                glm::vec2 mousePos(mouseX, mouseY);
                
                // Reset all to normal first
                std::function<void(engine::ui::UIComponent*)> resetState = [&](engine::ui::UIComponent* comp) {
                    if (comp->isInteractable()) {
                        comp->setState(engine::ui::UIState::Normal);
                    }
                    for (auto* child : comp->getChildren()) {
                        resetState(child);
                    }
                };
                resetState(root);
                
                // Set hover for components under mouse
                std::function<void(engine::ui::UIComponent*)> setHover = [&](engine::ui::UIComponent* comp) {
                    if (comp->isInteractable() && comp->containsPoint(mousePos)) {
                        comp->setState(engine::ui::UIState::Hover);
                    }
                    for (auto* child : comp->getChildren()) {
                        setHover(child);
                    }
                };
                setHover(root);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int mouseX = event.button.x;
                    int mouseY = event.button.y;
                    glm::vec2 mousePos(mouseX, mouseY);
                    
                    // Set active for components under mouse
                    std::function<void(engine::ui::UIComponent*)> setActive = [&](engine::ui::UIComponent* comp) {
                        if (comp->isInteractable() && comp->containsPoint(mousePos)) {
                            comp->setState(engine::ui::UIState::Active);
                            
                            // Test button click
                            if (auto* button = dynamic_cast<engine::ui::UIButton*>(comp)) {
                                std::cout << "Button clicked: " << button->getText() << std::endl;
                                if (button->isToggleMode()) {
                                    button->setToggled(!button->isToggled());
                                }
                            }
                            
                            // Test checkbox toggle
                            if (auto* checkbox = dynamic_cast<engine::ui::UICheckbox*>(comp)) {
                                checkbox->setChecked(!checkbox->isChecked());
                                std::cout << "Checkbox toggled: " << (checkbox->isChecked() ? "checked" : "unchecked") << std::endl;
                            }
                            
                            // Test slider drag
                            if (auto* slider = dynamic_cast<engine::ui::UISlider*>(comp)) {
                                glm::vec2 pos = comp->getWorldPosition();
                                glm::vec2 size = comp->getWorldSize();
                                float relativeX = (mouseX - pos.x) / size.x;
                                if (relativeX < 0.0f) relativeX = 0.0f;
                                if (relativeX > 1.0f) relativeX = 1.0f;
                                slider->setValue(relativeX);
                                std::cout << "Slider value: " << slider->getValue() << std::endl;
                            }
                        }
                        for (auto* child : comp->getChildren()) {
                            setActive(child);
                        }
                    };
                    setActive(root);
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int mouseX = event.button.x;
                    int mouseY = event.button.y;
                    glm::vec2 mousePos(mouseX, mouseY);
                    
                    // Reset to hover for components under mouse
                    std::function<void(engine::ui::UIComponent*)> resetToHover = [&](engine::ui::UIComponent* comp) {
                        if (comp->isInteractable()) {
                            if (comp->containsPoint(mousePos)) {
                                comp->setState(engine::ui::UIState::Hover);
                            } else {
                                comp->setState(engine::ui::UIState::Normal);
                            }
                        }
                        for (auto* child : comp->getChildren()) {
                            resetToHover(child);
                        }
                    };
                    resetToHover(root);
                }
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
        SDL_RenderClear(renderer);
        
        // Render UI
        uiRenderer.render(root);
        
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
