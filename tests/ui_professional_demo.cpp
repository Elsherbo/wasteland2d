// Professional UI Demo - Settings Panel
// Demonstrates all UI components with professional UI/UX design
#include <SDL.h>
#include <SDL_ttf.h>
#include <glm/vec2.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>

#include "ui/UIComponent.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UISlider.h"
#include "ui/UICheckbox.h"
#include "ui/UIVBox.h"
#include "ui/UIHBox.h"
#include "ui/UIGrid.h"
#include "ui/UISliderContainer.h"
#include "ui/UIScrollContainer.h"
#include "ui/UIRenderer.h"
#include "ui/UIStyle.h"
#include "ui/UIManager.h"
#include "ui/InputEvent.h"
#include "render/Font.h"
#include "render/TextRenderer.h"
#include "render/Color.h"

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 700;

// Helper to convert float to string
std::string floatToString(float value, int decimals = 0) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    return oss.str();
}

// Settings state
struct Settings {
    float resolution = 1.0f;       // 0-1, mapped to resolutions
    float quality = 2.0f;         // 0-3, Low/Medium/High/Ultra
    float masterVolume = 75.0f;    // 0-100
    float musicVolume = 60.0f;     // 0-100
    float sfxVolume = 80.0f;       // 0-100
    bool showFps = true;
    bool vsync = true;
    bool fullscreen = false;
    bool debugMode = false;
} settings;

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
        "Professional UI Demo - Settings Panel",
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
    engine::render::Font font("C:\\Windows\\Fonts\\arial.ttf", 14);
    engine::render::TextRenderer textRenderer(renderer);
    
    // Create UI manager
    engine::ui::UIManager uiManager;
    
    // Set up default theme with styles
    auto& themeManager = uiManager.getThemeManager();
    
    // Button style
    engine::ui::UIStyle buttonStyle;
    buttonStyle.backgroundColor[engine::ui::UIState::Normal] = glm::vec4(0.39f, 0.39f, 0.39f, 1.0f);
    buttonStyle.backgroundColor[engine::ui::UIState::Hover] = glm::vec4(0.59f, 0.59f, 0.59f, 1.0f);
    buttonStyle.backgroundColor[engine::ui::UIState::Active] = glm::vec4(0.78f, 0.78f, 0.78f, 1.0f);
    buttonStyle.textColor[engine::ui::UIState::Normal] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    buttonStyle.borderColor[engine::ui::UIState::Normal] = glm::vec4(0.78f, 0.78f, 0.78f, 1.0f);
    themeManager.registerStyle("Default", "Button", buttonStyle);
    
    // Label style
    engine::ui::UIStyle labelStyle;
    labelStyle.textColor[engine::ui::UIState::Normal] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    themeManager.registerStyle("Default", "Label", labelStyle);
    
    // Slider style
    engine::ui::UIStyle sliderStyle;
    sliderStyle.backgroundColor[engine::ui::UIState::Normal] = glm::vec4(0.39f, 0.39f, 0.39f, 1.0f);
    sliderStyle.backgroundColor[engine::ui::UIState::Hover] = glm::vec4(0.59f, 0.59f, 0.59f, 1.0f);
    sliderStyle.backgroundColor[engine::ui::UIState::Active] = glm::vec4(0.78f, 0.78f, 0.78f, 1.0f);
    themeManager.registerStyle("Default", "Slider", sliderStyle);
    
    // Checkbox style
    engine::ui::UIStyle checkboxStyle;
    checkboxStyle.backgroundColor[engine::ui::UIState::Normal] = glm::vec4(0.39f, 0.39f, 0.39f, 1.0f);
    checkboxStyle.backgroundColor[engine::ui::UIState::Hover] = glm::vec4(0.59f, 0.59f, 0.59f, 1.0f);
    checkboxStyle.backgroundColor[engine::ui::UIState::Active] = glm::vec4(0.78f, 0.78f, 0.78f, 1.0f);
    checkboxStyle.textColor[engine::ui::UIState::Normal] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    themeManager.registerStyle("Default", "Checkbox", checkboxStyle);
    
    // Container style
    engine::ui::UIStyle containerStyle;
    containerStyle.backgroundColor[engine::ui::UIState::Normal] = glm::vec4(0.12f, 0.12f, 0.14f, 0.59f);
    containerStyle.borderColor[engine::ui::UIState::Normal] = glm::vec4(0.39f, 0.39f, 0.39f, 1.0f);
    themeManager.registerStyle("Default", "Container", containerStyle);
    
    // Create UI renderer
    engine::ui::UIRenderer uiRenderer(renderer, textRenderer, font, themeManager);
    
    // Get UI layer
    engine::ui::UILayer* uiLayer = uiManager.getLayer("UI");
    
    // === STEP 1: Structure Setup ===
    auto mainContainer = std::make_unique<engine::ui::UIVBox>();
    mainContainer->setPosition(glm::vec2(50, 30));
    mainContainer->setSize(glm::vec2(800, 750));  // Increased from 700 to 750
    mainContainer->setSpacing(15);
    mainContainer->setPadding(glm::vec2(20));
    mainContainer->setInteractable(true);
    
    // Header
    auto headerLabel = std::make_unique<engine::ui::UILabel>();
    headerLabel->setText("Game Settings");
    headerLabel->setSize(glm::vec2(760, 30));
    headerLabel->setInteractable(true);
    mainContainer->addChild(std::move(headerLabel));
    
    // Separator line (image placeholder)
    auto separator = std::make_unique<engine::ui::UIImage>();
    separator->setSize(glm::vec2(760, 2));
    separator->setTintColor(glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
    separator->setInteractable(true);
    mainContainer->addChild(std::move(separator));
    
    // === STEP 2: Graphics Section ===
    auto graphicsSection = std::make_unique<engine::ui::UIVBox>();
    graphicsSection->setSpacing(10);
    graphicsSection->setPadding(glm::vec2(15));
    graphicsSection->setSizeFlags(engine::ui::SizeFlag::Fill);  // Fill width
    graphicsSection->setSize(glm::vec2(730, 130));
    graphicsSection->setInteractable(true);
    
    auto graphicsLabel = std::make_unique<engine::ui::UILabel>();
    graphicsLabel->setText("Graphics Settings");
    graphicsLabel->setSize(glm::vec2(730, 20));
    graphicsLabel->setInteractable(true);
    graphicsSection->addChild(std::move(graphicsLabel));
    
    // Resolution slider
    auto resolutionSlider = std::make_unique<engine::ui::UISliderContainer>();
    resolutionSlider->setLabelText("Resolution");
    resolutionSlider->setSize(glm::vec2(700, 30));
    resolutionSlider->setShowValueLabel(true);
    resolutionSlider->setSliderValue(settings.resolution);
    resolutionSlider->setInteractable(true);
    resolutionSlider->onValueChanged = [](float value) {
        settings.resolution = value;
        std::cout << "Resolution: " << settings.resolution << std::endl;
    };
    graphicsSection->addChild(std::move(resolutionSlider));
    
    // Quality slider
    auto qualitySlider = std::make_unique<engine::ui::UISliderContainer>();
    qualitySlider->setLabelText("Quality");
    qualitySlider->setSize(glm::vec2(700, 30));
    qualitySlider->setShowValueLabel(true);
    qualitySlider->setSliderValue(settings.quality / 3.0f);
    qualitySlider->setInteractable(true);
    qualitySlider->onValueChanged = [](float value) {
        settings.quality = value * 3.0f;
        std::cout << "Quality: " << settings.quality << std::endl;
    };
    graphicsSection->addChild(std::move(qualitySlider));
    
    // VSync checkbox
    auto vsyncCheckbox = std::make_unique<engine::ui::UICheckbox>();
    vsyncCheckbox->setText("VSync");
    vsyncCheckbox->setSize(glm::vec2(730, 25));
    vsyncCheckbox->setChecked(settings.vsync);
    vsyncCheckbox->onCheckedChanged = [](bool checked) {
        settings.vsync = checked;
        std::cout << "VSync: " << (checked ? "enabled" : "disabled") << std::endl;
    };
    graphicsSection->addChild(std::move(vsyncCheckbox));
    
    mainContainer->addChild(std::move(graphicsSection));
    
    // === STEP 3: Audio Section ===
    auto audioSection = std::make_unique<engine::ui::UIVBox>();
    audioSection->setSpacing(10);
    audioSection->setPadding(glm::vec2(15));
    audioSection->setSizeFlags(engine::ui::SizeFlag::Fill);  // Fill width
    audioSection->setSize(glm::vec2(730, 130));
    audioSection->setInteractable(true);
    
    auto audioLabel = std::make_unique<engine::ui::UILabel>();
    audioLabel->setText("Audio Settings");
    audioLabel->setSize(glm::vec2(730, 20));
    audioLabel->setInteractable(true);
    audioSection->addChild(std::move(audioLabel));
    
    // Master volume
    auto masterVolumeSlider = std::make_unique<engine::ui::UISliderContainer>();
    masterVolumeSlider->setLabelText("Master Volume");
    masterVolumeSlider->setSize(glm::vec2(700, 30));
    masterVolumeSlider->setShowValueLabel(true);
    masterVolumeSlider->setSliderValue(settings.masterVolume / 100.0f);
    masterVolumeSlider->setInteractable(true);
    masterVolumeSlider->onValueChanged = [](float value) {
        settings.masterVolume = value * 100.0f;
        std::cout << "Master Volume: " << settings.masterVolume << "%" << std::endl;
    };
    audioSection->addChild(std::move(masterVolumeSlider));
    
    // Music volume
    auto musicVolumeSlider = std::make_unique<engine::ui::UISliderContainer>();
    musicVolumeSlider->setLabelText("Music Volume");
    musicVolumeSlider->setSize(glm::vec2(700, 30));
    musicVolumeSlider->setShowValueLabel(true);
    musicVolumeSlider->setSliderValue(settings.musicVolume / 100.0f);
    musicVolumeSlider->setInteractable(true);
    musicVolumeSlider->onValueChanged = [](float value) {
        settings.musicVolume = value * 100.0f;
        std::cout << "Music Volume: " << settings.musicVolume << "%" << std::endl;
    };
    audioSection->addChild(std::move(musicVolumeSlider));
    
    // SFX volume
    auto sfxVolumeSlider = std::make_unique<engine::ui::UISliderContainer>();
    sfxVolumeSlider->setLabelText("SFX Volume");
    sfxVolumeSlider->setSize(glm::vec2(700, 30));
    sfxVolumeSlider->setShowValueLabel(true);
    sfxVolumeSlider->setSliderValue(settings.sfxVolume / 100.0f);
    sfxVolumeSlider->setInteractable(true);
    sfxVolumeSlider->onValueChanged = [](float value) {
        settings.sfxVolume = value * 100.0f;
        std::cout << "SFX Volume: " << settings.sfxVolume << "%" << std::endl;
    };
    audioSection->addChild(std::move(sfxVolumeSlider));
    
    mainContainer->addChild(std::move(audioSection));
    
    // === STEP 4: Options Section ===
    auto optionsSection = std::make_unique<engine::ui::UIVBox>();
    optionsSection->setSpacing(8);
    optionsSection->setPadding(glm::vec2(15));
    optionsSection->setSizeFlags(engine::ui::SizeFlag::Fill);  // Fill width
    optionsSection->setSize(glm::vec2(730, 100));
    optionsSection->setInteractable(true);
    
    auto optionsLabel = std::make_unique<engine::ui::UILabel>();
    optionsLabel->setText("Game Options");
    optionsLabel->setSize(glm::vec2(730, 20));
    optionsLabel->setInteractable(true);
    optionsSection->addChild(std::move(optionsLabel));
    
    auto showFpsCheckbox = std::make_unique<engine::ui::UICheckbox>();
    showFpsCheckbox->setText("Show FPS");
    showFpsCheckbox->setSize(glm::vec2(700, 25));
    showFpsCheckbox->setChecked(settings.showFps);
    showFpsCheckbox->onCheckedChanged = [](bool checked) {
        settings.showFps = checked;
        std::cout << "Show FPS: " << (checked ? "enabled" : "disabled") << std::endl;
    };
    optionsSection->addChild(std::move(showFpsCheckbox));
    
    auto fullscreenCheckbox = std::make_unique<engine::ui::UICheckbox>();
    fullscreenCheckbox->setText("Fullscreen Mode");
    fullscreenCheckbox->setSize(glm::vec2(700, 25));
    fullscreenCheckbox->setChecked(settings.fullscreen);
    fullscreenCheckbox->onCheckedChanged = [](bool checked) {
        settings.fullscreen = checked;
        std::cout << "Fullscreen: " << (checked ? "enabled" : "disabled") << std::endl;
    };
    optionsSection->addChild(std::move(fullscreenCheckbox));
    
    auto debugCheckbox = std::make_unique<engine::ui::UICheckbox>();
    debugCheckbox->setText("Enable Debug Mode");
    debugCheckbox->setSize(glm::vec2(700, 25));
    debugCheckbox->setChecked(settings.debugMode);
    debugCheckbox->onCheckedChanged = [](bool checked) {
        settings.debugMode = checked;
        std::cout << "Debug Mode: " << (checked ? "enabled" : "disabled") << std::endl;
    };
    optionsSection->addChild(std::move(debugCheckbox));
    
    mainContainer->addChild(std::move(optionsSection));
    
    // === STEP 5: Actions Section ===
    auto actionsSection = std::make_unique<engine::ui::UIHBox>();
    actionsSection->setSpacing(10);
    actionsSection->setPadding(glm::vec2(15));
    actionsSection->setSize(glm::vec2(730, 50));
    actionsSection->setInteractable(true);
    
    auto applyButton = std::make_unique<engine::ui::UIButton>();
    applyButton->setText("Apply");
    applyButton->setSize(glm::vec2(150, 35));
    applyButton->onClick = []() {
        std::cout << "=== SETTINGS APPLIED ===" << std::endl;
        std::cout << "Resolution: " << settings.resolution << std::endl;
        std::cout << "Quality: " << settings.quality << std::endl;
        std::cout << "Master Volume: " << settings.masterVolume << "%" << std::endl;
        std::cout << "Music Volume: " << settings.musicVolume << "%" << std::endl;
        std::cout << "SFX Volume: " << settings.sfxVolume << "%" << std::endl;
        std::cout << "Show FPS: " << (settings.showFps ? "enabled" : "disabled") << std::endl;
        std::cout << "VSync: " << (settings.vsync ? "enabled" : "disabled") << std::endl;
        std::cout << "Fullscreen: " << (settings.fullscreen ? "enabled" : "disabled") << std::endl;
        std::cout << "Debug Mode: " << (settings.debugMode ? "enabled" : "disabled") << std::endl;
    };
    actionsSection->addChild(std::move(applyButton));
    
    auto resetButton = std::make_unique<engine::ui::UIButton>();
    resetButton->setText("Reset");
    resetButton->setSize(glm::vec2(150, 35));
    resetButton->onClick = []() {
        std::cout << "Settings reset to current values" << std::endl;
    };
    actionsSection->addChild(std::move(resetButton));
    
    auto defaultsButton = std::make_unique<engine::ui::UIButton>();
    defaultsButton->setText("Defaults");
    defaultsButton->setSize(glm::vec2(150, 35));
    defaultsButton->onClick = []() {
        std::cout << "Settings reset to defaults" << std::endl;
        settings.resolution = 1.0f;
        settings.quality = 2.0f;
        settings.masterVolume = 75.0f;
        settings.musicVolume = 60.0f;
        settings.sfxVolume = 80.0f;
        settings.showFps = true;
        settings.vsync = true;
        settings.fullscreen = false;
        settings.debugMode = false;
        std::cout << "Defaults applied" << std::endl;
    };
    actionsSection->addChild(std::move(defaultsButton));
    
    mainContainer->addChild(std::move(actionsSection));
    
    // === STEP 6: Grid Demo ===
    auto gridLabel = std::make_unique<engine::ui::UILabel>();
    gridLabel->setText("Inventory Grid Demo");
    gridLabel->setSize(glm::vec2(760, 20));
    gridLabel->setInteractable(true);
    mainContainer->addChild(std::move(gridLabel));
    
    auto inventoryGrid = std::make_unique<engine::ui::UIGrid>();
    inventoryGrid->setColumns(3);
    inventoryGrid->setRows(4);  // Increased from 2 to 4
    inventoryGrid->setCellWidth(80.0f);
    inventoryGrid->setCellHeight(80.0f);
    inventoryGrid->setSpacing(5.0f);
    inventoryGrid->setSize(glm::vec2(250, 335));  // Increased height to fit 4 rows
    inventoryGrid->setInteractable(true);
    
    // Add 12 items to grid (4 rows x 3 columns)
    for (int i = 1; i <= 12; i++) {
        auto itemImage = std::make_unique<engine::ui::UIImage>();
        itemImage->setSize(glm::vec2(80, 80));
        // Alternate colors for visibility
        float colorVal = 0.4f + ((i % 2) * 0.2f);
        itemImage->setTintColor(glm::vec4(colorVal, colorVal, colorVal + 0.1f, 1.0f));
        itemImage->setInteractable(true);
        inventoryGrid->addChild(std::move(itemImage));
    }
    
    mainContainer->addChild(std::move(inventoryGrid));
    
    // === STEP 7: Scroll Demo ===
    auto scrollLabel = std::make_unique<engine::ui::UILabel>();
    scrollLabel->setText("Scrollable List Demo");
    scrollLabel->setSize(glm::vec2(760, 20));
    scrollLabel->setInteractable(true);
    mainContainer->addChild(std::move(scrollLabel));
    
    auto scrollContainer = std::make_unique<engine::ui::UIScrollContainer>();
    scrollContainer->setSize(glm::vec2(400, 100));
    scrollContainer->setContentSize(glm::vec2(400, 250));  // Increased from 150 to 250
    scrollContainer->setInteractable(true);
    scrollContainer->setVisible(true);
    
    auto scrollContent = std::make_unique<engine::ui::UIVBox>();
    scrollContent->setSpacing(5);
    scrollContent->setPadding(glm::vec2(10));
    scrollContent->setInteractable(true);
    scrollContent->setVisible(true);
    
    for (int i = 1; i <= 5; i++) {
        auto itemLabel = std::make_unique<engine::ui::UILabel>();
        itemLabel->setText("Item " + std::to_string(i) + ": " + std::string(i % 2 == 0 ? "Sword" : "Shield"));
        itemLabel->setSize(glm::vec2(380, 25));
        itemLabel->setInteractable(true);
        itemLabel->setVisible(true);
        scrollContent->addChild(std::move(itemLabel));
    }
    
    scrollContainer->addChild(std::move(scrollContent));
    mainContainer->addChild(std::move(scrollContainer));
    
    // Layout and add to layer
    mainContainer->layout();
    uiLayer->addComponent(mainContainer.get());
    
    // Keep ownership
    std::vector<std::unique_ptr<engine::ui::UIComponent>> components;
    components.push_back(std::move(mainContainer));
    
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
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseMotion(
                    static_cast<float>(event.motion.x),
                    static_cast<float>(event.motion.y),
                    static_cast<float>(event.motion.xrel),
                    static_cast<float>(event.motion.yrel)
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseButton(
                    static_cast<float>(event.button.x),
                    static_cast<float>(event.button.y),
                    event.button.button,
                    true
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseButton(
                    static_cast<float>(event.button.x),
                    static_cast<float>(event.button.y),
                    event.button.button,
                    false
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEWHEEL) {
                // Convert wheel delta (positive = scroll down)
                float delta = event.wheel.y > 0 ? 1.0f : -1.0f;
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseWheel(
                    static_cast<float>(event.wheel.mouseX),
                    static_cast<float>(event.wheel.mouseY),
                    delta
                );
                uiManager.dispatchInput(inputEvent);
            }
        }
        
        // Update UI
        uiManager.update(0.016);
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 26, 26, 46, 255);
        SDL_RenderClear(renderer);
        
        // Render UI
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
