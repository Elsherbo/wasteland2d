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

void UISlider::guiInput(const InputEvent& event) {
    if (!isInteractable() || getState() == UIState::Disabled) {
        return;
    }
    
    if (!event.isMouseButton() && !event.isMouseMotion()) return;
    
    const auto* mouseData = event.getMouseData();
    if (!mouseData) return;
    
    glm::vec2 thumbPos = getThumbPosition();
    glm::vec2 worldSize = getWorldSize();
    
    // Check if mouse is over thumb or track
    bool isOverThumb = mouseData->position.x >= thumbPos.x && 
                       mouseData->position.x <= thumbPos.x + thumbSize_ &&
                       mouseData->position.y >= thumbPos.y && 
                       mouseData->position.y <= thumbPos.y + thumbSize_;
    
    bool isOverTrack = containsPoint(mouseData->position);
    
    if (event.isMouseButton() && mouseData->pressed && (isOverThumb || isOverTrack)) {
        isDragging_ = true;
        setState(UIState::Active);
        
        // Update value based on mouse position
        glm::vec2 worldPos = getWorldPosition();
        float percent = (mouseData->position.x - worldPos.x) / (worldSize.x - thumbSize_);
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;
        setValue(minValue_ + percent * (maxValue_ - minValue_));
        
        acceptEvent();
        return;
    }
    
    if (event.isMouseButton() && !mouseData->pressed) {
        isDragging_ = false;
        if (isOverThumb || isOverTrack) {
            setState(UIState::Hover);
        } else {
            setState(UIState::Normal);
        }
        acceptEvent();
        return;
    }
    
    if (event.isMouseMotion() && isDragging_) {
        // Update value while dragging
        glm::vec2 worldPos = getWorldPosition();
        float percent = (mouseData->position.x - worldPos.x) / (worldSize.x - thumbSize_);
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;
        setValue(minValue_ + percent * (maxValue_ - minValue_));
        acceptEvent();
        return;
    }
    
    // Update hover state
    if (isOverThumb || isOverTrack) {
        setState(UIState::Hover);
    } else {
        setState(UIState::Normal);
    }
}

} // namespace engine::ui
