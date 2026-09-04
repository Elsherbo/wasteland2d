#include "UISlider.h"
#include "InputEvent.h"
#include "UIRenderer.h"
#include <algorithm>

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
    
    // Calculate thumb position based on value. Guard against minValue_ ==
    // maxValue_ (a single-option slider, or one constructed before its
    // range is configured) -- that division is otherwise a 0/0 NaN that
    // propagates straight into the render position.
    float range = maxValue_ - minValue_;
    float percent = (range != 0.0f) ? (value_ - minValue_) / range : 0.0f;
    float trackSpan = std::max(0.0f, worldSize.x - thumbSize_);
    float thumbX = worldPos.x + percent * trackSpan;
    float thumbY = worldPos.y + (worldSize.y - thumbSize_) * 0.5f;
    
    return glm::vec2(thumbX, thumbY);
}

void UISlider::renderUI(UIRenderer& uiRenderer) {
    uiRenderer.renderSliderHelper(this);
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
    
    // Handle mouse motion for hover state and drag
    if (event.isMouseMotion()) {
        if (wasPressed_) {
            // Update value while dragging. Guard the denominator: if the
            // track is exactly as wide as the thumb (or narrower, e.g.
            // during an initial zero-size layout pass), this would
            // otherwise be a divide-by-zero.
            glm::vec2 worldPos = getWorldPosition();
            float trackSpan = worldSize.x - thumbSize_;
            float percent = (trackSpan > 0.0f) ? (mouseData->position.x - worldPos.x) / trackSpan : 0.0f;
            if (percent < 0.0f) percent = 0.0f;
            if (percent > 1.0f) percent = 1.0f;
            setValue(minValue_ + percent * (maxValue_ - minValue_));
            acceptEvent();
            return;
        }
        
        // Update hover state (only if not dragging)
        if (isOverThumb || isOverTrack) {
            setState(UIState::Hover);
        } else {
            setState(UIState::Normal);
        }
        return;
    }
    
    // Handle mouse button events
    if (!event.isMouseButton()) return;
    
    if (mouseData->pressed && (isOverThumb || isOverTrack)) {
        wasPressed_ = true;
        setState(UIState::Active);
        
        // Update value based on mouse position (same guard as the drag path).
        glm::vec2 worldPos = getWorldPosition();
        float trackSpan = worldSize.x - thumbSize_;
        float percent = (trackSpan > 0.0f) ? (mouseData->position.x - worldPos.x) / trackSpan : 0.0f;
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;
        setValue(minValue_ + percent * (maxValue_ - minValue_));
        
        acceptEvent();
        return;
    }
    
    if (!mouseData->pressed && wasPressed_) {
        wasPressed_ = false;
        if (isOverThumb || isOverTrack) {
            setState(UIState::Hover);
        } else {
            setState(UIState::Normal);
        }
        acceptEvent();
        return;
    }
}

glm::vec2 UISlider::calculateMinSize() const {
    // Slider needs minimum size for usability
    float minWidth = 50.0f;
    float minHeight = 20.0f;
    
    // Respect custom minimum size if set
    glm::vec2 calculatedMin(minWidth, minHeight);
    if (customMinimumSize_.x > 0.0f) calculatedMin.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) calculatedMin.y = customMinimumSize_.y;
    
    return calculatedMin;
}

} // namespace engine::ui
