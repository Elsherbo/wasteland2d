// Test basic UI components
#include "ui/UIComponent.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UISlider.h"
#include "ui/UICheckbox.h"
#include "core/Logger.h"
#include <glm/vec4.hpp>
#include <cassert>

using namespace engine::ui;

int main() {
    // Initialize logger
    engine::LoggerConfig config;
    config.level = engine::LogLevel::Info;
    config.enableColors = true;
    engine::Logger::init(config);
    
    LOG_INFO(engine::LogCategory::UI, "Testing Basic UI Components...");
    
    // Test UIButton
    LOG_INFO(engine::LogCategory::UI, "--- UIButton ---");
    auto button = std::make_unique<UIButton>();
    assert(button != nullptr && "Button should be created");
    assert(button->isInteractable() && "Button should be interactable");
    button->setText("Click Me");
    assert(button->getText() == "Click Me" && "Text should be set");
    button->setNormalImage("button_normal.png");
    assert(button->getNormalImage() == "button_normal.png" && "Normal image should be set");
    button->setToggleMode(true);
    assert(button->isToggleMode() && "Toggle mode should be enabled");
    button->setToggled(true);
    assert(button->isToggled() && "Button should be toggled");
    LOG_INFO(engine::LogCategory::UI, "[ok] UIButton works");
    
    // Test UILabel
    LOG_INFO(engine::LogCategory::UI, "--- UILabel ---");
    auto label = std::make_unique<UILabel>();
    assert(label != nullptr && "Label should be created");
    assert(!label->isInteractable() && "Label should not be interactable");
    label->setText("Hello World");
    assert(label->getText() == "Hello World" && "Text should be set");
    label->setWordWrap(true);
    assert(label->isWordWrap() && "Word wrap should be enabled");
    label->setAlignment(TextAlignment::Center);
    assert(label->getAlignment() == TextAlignment::Center && "Alignment should be center");
    LOG_INFO(engine::LogCategory::UI, "[ok] UILabel works");
    
    // Test UIImage
    LOG_INFO(engine::LogCategory::UI, "--- UIImage ---");
    auto image = std::make_unique<UIImage>();
    assert(image != nullptr && "Image should be created");
    assert(!image->isInteractable() && "Image should not be interactable");
    image->setTexturePath("icon.png");
    assert(image->getTexturePath() == "icon.png" && "Texture path should be set");
    image->setMode(ImageMode::Fill);
    assert(image->getMode() == ImageMode::Fill && "Mode should be fill");
    image->setPreserveAspect(false);
    assert(!image->preserveAspect() && "Preserve aspect should be false");
    image->setTintColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    assert(image->getTintColor().r == 1.0f && "Tint color should be set");
    LOG_INFO(engine::LogCategory::UI, "[ok] UIImage works");
    
    // Test UISlider
    LOG_INFO(engine::LogCategory::UI, "--- UISlider ---");
    auto slider = std::make_unique<UISlider>();
    assert(slider != nullptr && "Slider should be created");
    assert(slider->isInteractable() && "Slider should be interactable");
    assert(slider->getValue() == 0.5f && "Default value should be 0.5");
    slider->setValue(0.75f);
    assert(slider->getValue() == 0.75f && "Value should be 0.75");
    slider->setMinValue(0.0f);
    slider->setMaxValue(100.0f);
    assert(slider->getMinValue() == 0.0f && "Min value should be 0.0");
    assert(slider->getMaxValue() == 100.0f && "Max value should be 100.0");
    slider->setTrackImage("track.png");
    assert(slider->getTrackImage() == "track.png" && "Track image should be set");
    slider->setThumbImage("thumb.png");
    assert(slider->getThumbImage() == "thumb.png" && "Thumb image should be set");
    slider->setThumbSize(20.0f);
    assert(slider->getThumbSize() == 20.0f && "Thumb size should be 20.0");
    LOG_INFO(engine::LogCategory::UI, "[ok] UISlider works");
    
    // Test UICheckbox
    LOG_INFO(engine::LogCategory::UI, "--- UICheckbox ---");
    auto checkbox = std::make_unique<UICheckbox>();
    assert(checkbox != nullptr && "Checkbox should be created");
    assert(checkbox->isInteractable() && "Checkbox should be interactable");
    assert(!checkbox->isChecked() && "Checkbox should not be checked by default");
    checkbox->setChecked(true);
    assert(checkbox->isChecked() && "Checkbox should be checked");
    checkbox->setText("Enable Feature");
    assert(checkbox->getText() == "Enable Feature" && "Text should be set");
    checkbox->setUncheckedImage("unchecked.png");
    assert(checkbox->getUncheckedImage() == "unchecked.png" && "Unchecked image should be set");
    checkbox->setCheckedImage("checked.png");
    assert(checkbox->getCheckedImage() == "checked.png" && "Checked image should be set");
    LOG_INFO(engine::LogCategory::UI, "[ok] UICheckbox works");
    
    LOG_INFO(engine::LogCategory::UI, "ALL BASIC UI COMPONENT TESTS PASSED");
    
    engine::Logger::shutdown();
    return 0;
}
