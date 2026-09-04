#include "UISliderContainer.h"
#include "UIComponent.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace engine::ui {

UISliderContainer::UISliderContainer() {
    // Create components
    label_ = new UILabel();
    slider_ = new UISlider();
    valueLabel_ = new UILabel();
    
    // Add as children
    addChild(std::unique_ptr<UIComponent>(label_));
    addChild(std::unique_ptr<UIComponent>(slider_));
    addChild(std::unique_ptr<UIComponent>(valueLabel_));
    
    // Set fixed size for value label to prevent layout shifts
    valueLabel_->setSize(glm::vec2(50.0f, 20.0f));
    valueLabel_->setCustomMinimumSize(glm::vec2(50.0f, 20.0f));
    valueLabel_->setAlignment(TextAlignment::Center);  // e.g. "75%" centered in its column, not stuck to the left edge
    
    // Set up slider callback to update value label and call external callback
    slider_->onValueChanged = [this](float value) {
        this->updateValueLabel();
        // Call external callback if set
        if (this->onValueChanged) {
            this->onValueChanged(value);
        }
    };
    
    // Set initial layout
    setLayoutDirty();
    
    // Initialize cache
    lastValueText_ = "";
}

void UISliderContainer::setLabelText(const std::string& text) {
    if (label_) {
        label_->setText(text);
    }
}

void UISliderContainer::setShowValueLabel(bool show) {
    if (valueLabel_) {
        valueLabel_->setVisible(show);
    }
}

void UISliderContainer::setSliderValue(float value) {
    if (slider_) {
        slider_->setValue(value);
        updateValueLabel();
    }
}

void UISliderContainer::updateValueLabel() {
    if (valueLabel_ && slider_) {
        float value = slider_->getValue();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << (value * 100.0f);
        
        std::string newText = oss.str() + "%";
        
        // Only update text if it changed (prevents unnecessary updates and layout recalculations)
        if (lastValueText_ != newText) {
            lastValueText_ = newText;
            valueLabel_->setText(newText);
        }
    }
}

void UISliderContainer::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    // Update value label before layout
    updateValueLabel();
    
    glm::vec2 contentArea = getContentArea();
    
    // Layout: [Label] [Slider] [Value]
    float currentX = padding_.x;
    float y = padding_.y;
    
    // Position label
    if (label_ && label_->isVisible()) {
        // label_ (unlike slider_/valueLabel_ below) never had its size set
        // here -- only its position -- so it was stuck at UIComponent's
        // default 100x100. That was harmless while label rendering was
        // top-aligned, but now that rendering vertically centers text
        // within getWorldSize(), that phantom 100px-tall box pushed the
        // text roughly one whole row downward.
        //
        // The box is inset by kLabelLeftPadding and narrowed to match, so
        // the left-aligned text sits a few px in from the row's edge --
        // matching the breathing room the (centered) value text already
        // has on the right -- without moving where the slider itself
        // starts (currentX below still advances by the full kLabelWidth).
        label_->setSize(glm::vec2(kLabelWidth - kLabelLeftPadding, contentArea.y - padding_.y * 2.0f));
        label_->setPosition(glm::vec2(currentX + kLabelLeftPadding, y));
        currentX += kLabelWidth + kSpacing;
    }
    
    // Position slider (take remaining space)
    if (slider_ && slider_->isVisible()) {
        float sliderWidth = contentArea.x - currentX - padding_.x;
        if (valueLabel_ && valueLabel_->isVisible()) {
            sliderWidth -= kValueLabelWidth + kSpacing;
        }
        float sliderHeight = contentArea.y - padding_.y * 2.0f;
        if (sliderHeight < 20.0f) sliderHeight = 20.0f;  // Minimum height
        slider_->setPosition(glm::vec2(currentX, y));
        slider_->setSize(glm::vec2(sliderWidth, sliderHeight));
        currentX += sliderWidth + kSpacing;
    }
    
    // Position value label
    if (valueLabel_ && valueLabel_->isVisible()) {
        valueLabel_->setSize(glm::vec2(kValueLabelWidth, contentArea.y - padding_.y * 2.0f));
        valueLabel_->setPosition(glm::vec2(currentX, y));
    }
    
    clearLayoutDirty();
}

glm::vec2 UISliderContainer::calculateMinSize() const {
    // No override existed here at all before -- UIComponent's base default
    // (customMinimumSize_ or else the dead, always-zero minSize_ field)
    // meant every UISliderContainer reported (0,0) regardless of its real
    // on-screen size. That's invisible to any ancestor VBox doing
    // bottom-up width accounting: a section containing a 700px-wide
    // slider row would compute its own min width from its *other*
    // children only, then get Fill-stretched to a parent that was in
    // turn auto-sized without ever having seen that 700px in the first
    // place -- exactly what produced sections narrower than their own
    // slider rows. Mirrors layout()'s own [label][slider][value] math so
    // the two can't disagree with each other.
    float width = padding_.x * 2.0f;
    float height = 0.0f;
    
    if (label_ && label_->isVisible()) {
        width += kLabelWidth + kSpacing;
        height = std::max(height, label_->calculateMinSize().y);
    }
    if (slider_ && slider_->isVisible()) {
        width += kMinSliderWidth + kSpacing;
        height = std::max(height, slider_->calculateMinSize().y);
    }
    if (valueLabel_ && valueLabel_->isVisible()) {
        width += kValueLabelWidth;
        height = std::max(height, valueLabel_->calculateMinSize().y);
    }
    
    height = std::max(height, 20.0f) + padding_.y * 2.0f;
    
    glm::vec2 minSize(width, height);
    if (customMinimumSize_.x > 0.0f) minSize.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) minSize.y = customMinimumSize_.y;
    
    return minSize;
}

} // namespace engine::ui
