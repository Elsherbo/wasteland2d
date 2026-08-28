#include "UISliderContainer.h"
#include "UIComponent.h"

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
    
    // Set initial layout
    setLayoutDirty();
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

void UISliderContainer::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    glm::vec2 contentArea = getContentArea();
    float spacing = 5.0f;  // Default spacing
    
    // Layout: [Label] [Slider] [Value]
    float currentX = padding_.x;
    float y = padding_.y;
    
    // Position label
    if (label_ && label_->isVisible()) {
        label_->setPosition(glm::vec2(currentX, y));
        currentX += label_->getSize().x + spacing;
    }
    
    // Position slider (take remaining space)
    if (slider_ && slider_->isVisible()) {
        float sliderWidth = contentArea.x - currentX - padding_.x;
        if (valueLabel_ && valueLabel_->isVisible()) {
            sliderWidth -= valueLabel_->getSize().x + spacing;
        }
        slider_->setPosition(glm::vec2(currentX, y));
        slider_->setSize(glm::vec2(sliderWidth, slider_->getSize().y));
        currentX += sliderWidth + spacing;
    }
    
    // Position value label
    if (valueLabel_ && valueLabel_->isVisible()) {
        valueLabel_->setPosition(glm::vec2(currentX, y));
    }
    
    clearLayoutDirty();
    
    // Call layout on children
    UIContainer::layout();
}

} // namespace engine::ui
