#include "ui/PauseMenu.h"

#include <algorithm>
#include <glm/vec2.hpp>
#include <SDL.h>

#include "core/Logger.h"
#include "core/Window.h"
#include "ui/UIVBox.h"
#include "ui/UIHBox.h"
#include "ui/UIScrollContainer.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UICheckbox.h"
#include "ui/UISliderContainer.h"
#include "ui/UIDefaultTheme.h"
#include "ui/InputEvent.h"

namespace game::ui {

PauseMenu::PauseMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight,
                      const std::string& fontPath, std::function<void()> onQuit,
                      engine::Window* window, engine::audio::AudioManager* audio)
    : onQuit_(std::move(onQuit)),
      window_(window),
      audio_(audio),
      font_(fontPath, 14),
      headingFont_(fontPath, 20),
      textRenderer_(renderer),
      uiRenderer_(renderer, textRenderer_, font_, uiManager_.getThemeManager(), &headingFont_) {
    engine::ui::installDefaultDarkTheme(uiManager_.getThemeManager(), uiRenderer_);
    buildUI(screenWidth, screenHeight);
}

void PauseMenu::buildUI(int screenWidth, int screenHeight) {
    using namespace engine::ui;
    
    const float kPanelWidth = 480.0f;
    const float kPanelHeight = std::min(560.0f, static_cast<float>(screenHeight) - 80.0f);
    
    // Outer panel: a ScrollContainer so the menu holds together even if
    // a future section (graphics resolution list, keybind rows, ...)
    // pushes total content past kPanelHeight -- exactly the failure
    // mode ui_professional_demo.cpp hit (and got fixed) for not having
    // this from the start.
    auto panel = std::make_unique<UIScrollContainer>();
    panel->setPosition(glm::vec2(
        (screenWidth - kPanelWidth) / 2.0f,
        (screenHeight - kPanelHeight) / 2.0f));
    panel->setSize(glm::vec2(kPanelWidth, kPanelHeight));
    panel->setPadding(glm::vec2(24));
    panel->setInteractable(true);
    panel->setStyleName("Panel");
    
    auto content = std::make_unique<UIVBox>();
    content->setSpacing(16);
    content->setAutoSize(true);
    content->setInteractable(true);
    content->setStyleName("Transparent");
    
    auto title = std::make_unique<UILabel>();
    title->setText("Paused");
    title->setSize(glm::vec2(kPanelWidth - 48.0f, 30));
    title->setHeading(true);
    title->setInteractable(true);
    content->addChild(std::move(title));
    
    // --- Graphics section ------------------------------------------
    auto graphicsSection = std::make_unique<UIVBox>();
    graphicsSection->setSpacing(10);
    graphicsSection->setPadding(glm::vec2(15));
    graphicsSection->setSizeFlags(SizeFlag::Fill);
    graphicsSection->setInteractable(true);
    graphicsSection->setStyleName("Section");
    
    auto graphicsLabel = std::make_unique<UILabel>();
    graphicsLabel->setText("Graphics");
    graphicsLabel->setSize(glm::vec2(380, 24));
    graphicsLabel->setHeading(true);
    graphicsLabel->setInteractable(true);
    graphicsSection->addChild(std::move(graphicsLabel));
    
    auto vsyncCheckbox = std::make_unique<UICheckbox>();
    vsyncCheckbox->setText("VSync");
    vsyncCheckbox->setSize(glm::vec2(380, 25));
    // Matches Window.cpp's actual current behavior (it already requests
    // SDL_RENDERER_PRESENTVSYNC) -- this checkbox doesn't control that
    // yet, but its default should still reflect reality rather than
    // silently disagreeing with what the game is actually doing.
    vsyncCheckbox->setChecked(true);
    vsyncCheckbox->setInteractable(true);
    vsyncCheckbox->onCheckedChanged = [this](bool checked) {
        LOG_INFO(engine::LogCategory::Core, "VSync checkbox: {} (requires restart for effect)", checked ? "enabled" : "disabled");
        // Note: VSync changes require renderer recreation which breaks UI
        // For now, just log the request without changing state
    };
    graphicsSection->addChild(std::move(vsyncCheckbox));
    
    auto fullscreenCheckbox = std::make_unique<UICheckbox>();
    fullscreenCheckbox->setText("Fullscreen");
    fullscreenCheckbox->setSize(glm::vec2(380, 25));
    fullscreenCheckbox->setInteractable(true);
    fullscreenCheckbox->onCheckedChanged = [this](bool checked) {
        if (window_) {
            window_->setFullscreen(checked);
            // Rebuild UI after fullscreen change to handle new dimensions
            if (window_->handle()) {
                int w, h;
                SDL_GetWindowSize(window_->handle(), &w, &h);
                rebuildUI(w, h);
            }
        }
    };
    graphicsSection->addChild(std::move(fullscreenCheckbox));
    
    content->addChild(std::move(graphicsSection));
    
    // --- Audio section -----------------------------------------------
    auto audioSection = std::make_unique<UIVBox>();
    audioSection->setSpacing(10);
    audioSection->setPadding(glm::vec2(15));
    audioSection->setSizeFlags(SizeFlag::Fill);
    audioSection->setInteractable(true);
    audioSection->setStyleName("Section");
    
    auto audioLabel = std::make_unique<UILabel>();
    audioLabel->setText("Audio");
    audioLabel->setSize(glm::vec2(380, 24));
    audioLabel->setHeading(true);
    audioLabel->setInteractable(true);
    audioSection->addChild(std::move(audioLabel));
    
    auto masterVolume = std::make_unique<UISliderContainer>();
    masterVolume->setLabelText("Master Volume");
    masterVolume->setSize(glm::vec2(380, 30));
    masterVolume->getSlider()->setValue(audio_ ? audio_->getMasterVolume() : 1.0f);
    masterVolume->setInteractable(true);
    masterVolume->onValueChanged = [this](float value) {
        if (audio_) {
            audio_->setMasterVolume(value);
        }
    };
    audioSection->addChild(std::move(masterVolume));
    
    auto musicVolume = std::make_unique<UISliderContainer>();
    musicVolume->setLabelText("Music Volume");
    musicVolume->setSize(glm::vec2(380, 30));
    musicVolume->getSlider()->setValue(audio_ ? audio_->getMusicVolume() : 0.7f);
    musicVolume->setInteractable(true);
    musicVolume->onValueChanged = [this](float value) {
        if (audio_) {
            audio_->setMusicVolume(value);
        }
    };
    audioSection->addChild(std::move(musicVolume));
    
    auto sfxVolume = std::make_unique<UISliderContainer>();
    sfxVolume->setLabelText("SFX Volume");
    sfxVolume->setSize(glm::vec2(380, 30));
    sfxVolume->getSlider()->setValue(audio_ ? audio_->getSFXVolume() : 0.8f);
    sfxVolume->setInteractable(true);
    sfxVolume->onValueChanged = [this](float value) {
        if (audio_) {
            audio_->setSFXVolume(value);
        }
    };
    audioSection->addChild(std::move(sfxVolume));
    
    content->addChild(std::move(audioSection));
    
    // --- Actions row ---------------------------------------------------
    auto actionsRow = std::make_unique<UIHBox>();
    actionsRow->setSpacing(12);
    actionsRow->setInteractable(true);
    actionsRow->setStyleName("Transparent");
    
    auto resumeButton = std::make_unique<UIButton>();
    resumeButton->setText("Resume");
    resumeButton->setSize(glm::vec2(160, 36));
    resumeButton->setStyleName("Button.Primary");
    resumeButton->setInteractable(true);
    resumeButton->onClick = [this]() {
        LOG_INFO(engine::LogCategory::Core, "Resume button clicked");
        setOpen(false);
    };
    actionsRow->addChild(std::move(resumeButton));
    
    auto quitButton = std::make_unique<UIButton>();
    quitButton->setText("Quit to Desktop");
    quitButton->setSize(glm::vec2(180, 36));
    quitButton->setStyleName("Button.Ghost");
    quitButton->setInteractable(true);
    quitButton->onClick = [this]() {
        if (onQuit_) onQuit_();
    };
    actionsRow->addChild(std::move(quitButton));
    
    content->addChild(std::move(actionsRow));
    
    panel->addChild(std::move(content));
    root_ = std::move(panel);
    
    // Register with the "UI" layer -- without this, UIManager::update()
    // has nothing to call layout() on (every child stays at its default
    // unlaidout position, which is exactly why everything piled up near
    // one spot instead of stacking down the panel), and
    // dispatchInput() has nothing to route clicks/hover to either.
    // Both demos do this; this was a plain omission here.
    uiManager_.getLayer("UI")->addComponent(root_.get());
}

void PauseMenu::rebuildUI(int screenWidth, int screenHeight) {
    // Remove old root from layer
    if (root_) {
        uiManager_.getLayer("UI")->removeComponent(root_.get());
    }
    
    // Rebuild UI with new dimensions
    buildUI(screenWidth, screenHeight);
}

void PauseMenu::handleInput(engine::InputManager& input) {
    if (!open_) return;
    
    int mouseX = 0;
    int mouseY = 0;
    input.getMousePosition(mouseX, mouseY);
    
    if (mouseX != lastMouseX_ || mouseY != lastMouseY_) {
        auto motion = engine::ui::InputEvent::mouseMotion(
            static_cast<float>(mouseX), static_cast<float>(mouseY),
            static_cast<float>(mouseX - lastMouseX_), static_cast<float>(mouseY - lastMouseY_));
        uiManager_.dispatchInput(motion);
        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;
    }
    
    if (input.wasMouseButtonPressed(SDL_BUTTON_LEFT)) {
        auto down = engine::ui::InputEvent::mouseButton(
            static_cast<float>(mouseX), static_cast<float>(mouseY), SDL_BUTTON_LEFT, true);
        uiManager_.dispatchInput(down);
    }
    if (input.wasMouseButtonReleased(SDL_BUTTON_LEFT)) {
        auto up = engine::ui::InputEvent::mouseButton(
            static_cast<float>(mouseX), static_cast<float>(mouseY), SDL_BUTTON_LEFT, false);
        uiManager_.dispatchInput(up);
    }
    
    int wheel = input.mouseWheelDelta();
    if (wheel != 0) {
        auto wheelEvent = engine::ui::InputEvent::mouseWheel(
            static_cast<float>(mouseX), static_cast<float>(mouseY), static_cast<float>(wheel));
        uiManager_.dispatchInput(wheelEvent);
    }
}

void PauseMenu::update(double dt) {
    if (!open_) return;
    uiManager_.update(dt);
}

void PauseMenu::render() {
    if (!open_) return;
    if (root_) {
        uiRenderer_.render(root_.get());
    }
}

} // namespace game::ui