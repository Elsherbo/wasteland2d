#include "UICheckbox.h"

namespace engine::ui {

UICheckbox::UICheckbox() {
    setInteractable(true);
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
    
    if (!event.isMouseButton()) return;
    
    const auto* mouseData = event.getMouseData();
    if (!mouseData) return;
    
    bool isOver = containsPoint(mouseData->position);
    
    if (mouseData->pressed && isOver) {
        setState(UIState::Active);
        acceptEvent();
        return;
    }
    
    if (!mouseData->pressed && isOver && getState() == UIState::Active) {
        setChecked(!checked_);
        setState(UIState::Hover);
        acceptEvent();
        return;
    }
    
    if (!mouseData->pressed && !isOver && getState() == UIState::Active) {
        setState(UIState::Normal);
        acceptEvent();
        return;
    }
    
    // Update hover state
    if (isOver && getState() != UIState::Active) {
        setState(UIState::Hover);
    } else if (!isOver && getState() != UIState::Active) {
        setState(UIState::Normal);
    }
}

} // namespace engine::ui
