#include "UISlider.h"

namespace engine::ui {

UISlider::UISlider() {
    setInteractable(true);
}

void UISlider::setValue(float value) {
    value_ = value;
    clampValue();
    setDirty();
    
    if (onValueChanged) {
        onValueChanged(value_);
    }
}

void UISlider::clampValue() {
    if (value_ < minValue_) value_ = minValue_;
    if (value_ > maxValue_) value_ = maxValue_;
}

glm::vec2 UISlider::getThumbPosition() const {
    glm::vec2 worldPos = getWorldPosition();
    glm::vec2 worldSize = getWorldSize();
    
    // Calculate thumb position based on value
    float percent = (value_ - minValue_) / (maxValue_ - minValue_);
    float thumbX = worldPos.x + percent * (worldSize.x - thumbSize_);
    float thumbY = worldPos.y + (worldSize.y - thumbSize_) * 0.5f;
    
    return glm::vec2(thumbX, thumbY);
}

void UISlider::render(Renderer& renderer) {
    // TODO: Implement rendering in Phase 6 with actual Renderer
    // For now, this is a placeholder
    (void)renderer;
    
    // Pseudo-code for rendering:
    // 1. Render track (image or color)
    // 2. Calculate thumb position
    // 3. Render thumb (image or color)
    // 4. Apply style states
}

bool UISlider::handleInput(const InputEvent& event) {
    if (!isInteractable() || getState() == UIState::Disabled) {
        return false;
    }
    
    glm::vec2 thumbPos = getThumbPosition();
    glm::vec2 worldSize = getWorldSize();
    
    // Check if mouse is over thumb or track
    bool isOverThumb = event.mousePosition.x >= thumbPos.x && 
                       event.mousePosition.x <= thumbPos.x + thumbSize_ &&
                       event.mousePosition.y >= thumbPos.y && 
                       event.mousePosition.y <= thumbPos.y + thumbSize_;
    
    bool isOverTrack = containsPoint(event.mousePosition);
    
    if (event.mouseDown && (isOverThumb || isOverTrack)) {
        isDragging_ = true;
        setState(UIState::Active);
        
        // Update value based on mouse position
        glm::vec2 worldPos = getWorldPosition();
        float percent = (event.mousePosition.x - worldPos.x) / (worldSize.x - thumbSize_);
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;
        setValue(minValue_ + percent * (maxValue_ - minValue_));
        
        return true;
    }
    
    if (event.mouseUp) {
        isDragging_ = false;
        if (isOverThumb || isOverTrack) {
            setState(UIState::Hover);
        } else {
            setState(UIState::Normal);
        }
        return true;
    }
    
    if (isDragging_) {
        // Update value while dragging
        glm::vec2 worldPos = getWorldPosition();
        float percent = (event.mousePosition.x - worldPos.x) / (worldSize.x - thumbSize_);
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;
        setValue(minValue_ + percent * (maxValue_ - minValue_));
        return true;
    }
    
    // Update hover state
    if (isOverThumb || isOverTrack) {
        setState(UIState::Hover);
    } else {
        setState(UIState::Normal);
    }
    
    return false;
}

} // namespace engine::ui
