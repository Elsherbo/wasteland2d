#include "UICheckbox.h"
#include "InputEvent.h"
#include "UIRenderer.h"

namespace engine::ui {

UICheckbox::UICheckbox() {
    setInteractable(true);
    wasPressed_ = false;
}

void UICheckbox::setChecked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        setDirty();
        
        if (onCheckedChanged) {
            onCheckedChanged(checked_);
        }
    }
}

void UICheckbox::renderUI(UIRenderer& uiRenderer) {
    uiRenderer.renderCheckboxHelper(this);
}

void UICheckbox::render(Renderer& renderer) {
    // TODO: Implement rendering in Phase 6 with actual Renderer
    // For now, this is a placeholder
    (void)renderer;
    
    // Pseudo-code for rendering:
    // 1. Render checkbox (image or color) based on checked state
    // 2. Render checkmark if checked
    // 3. Render text label
    // 4. Apply style states
}

void UICheckbox::guiInput(const InputEvent& event) {
    if (!isInteractable() || getState() == UIState::Disabled) {
        return;
    }
    
    const auto* mouseData = event.getMouseData();
    if (!mouseData) return;
    
    bool isOver = containsPoint(mouseData->position);
    
    // Handle mouse motion for hover state
    if (event.isMouseMotion()) {
        if (wasPressed_) {
            // While pressed, stay in Active if over, go to Normal if not over
            if (!isOver) {
                setState(UIState::Normal);
            }
        } else {
            // Not pressed, handle normal hover
            if (isOver) {
                setState(UIState::Hover);
            } else {
                setState(UIState::Normal);
            }
        }
        return;
    }
    
    // Handle mouse button events
    if (!event.isMouseButton()) return;
    
    if (mouseData->pressed && isOver) {
        setState(UIState::Active);
        wasPressed_ = true;
        acceptEvent();
        return;
    }
    
    if (!mouseData->pressed && wasPressed_) {
        wasPressed_ = false;
        
        if (isOver) {
            setChecked(!checked_);
            setState(UIState::Hover);
            acceptEvent();
            return;
        } else {
            setState(UIState::Normal);
            acceptEvent();
            return;
        }
    }
}

glm::vec2 UICheckbox::calculateMinSize() const {
    // Calculate minimum size based on text length + checkbox box
    float estimatedWidth = static_cast<float>(text_.length() * 10) + 30.0f;  // +30px for checkbox
    float estimatedHeight = 25.0f;  // Default checkbox height
    
    // Respect custom minimum size if set
    glm::vec2 calculatedMin(estimatedWidth, estimatedHeight);
    if (customMinimumSize_.x > 0.0f) calculatedMin.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) calculatedMin.y = customMinimumSize_.y;
    
    // Ensure minimum size
    if (calculatedMin.x < 80.0f) calculatedMin.x = 80.0f;
    if (calculatedMin.y < 25.0f) calculatedMin.y = 25.0f;
    
    return calculatedMin;
}

} // namespace engine::ui
