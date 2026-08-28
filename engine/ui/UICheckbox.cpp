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

bool UICheckbox::handleInput(const InputEvent& event) {
    if (!isInteractable() || getState() == UIState::Disabled) {
        return false;
    }
    
    bool isOver = containsPoint(event.mousePosition);
    
    if (event.mouseDown && isOver) {
        setState(UIState::Active);
        return true;
    }
    
    if (event.mouseUp && isOver) {
        setChecked(!checked_);
        setState(UIState::Hover);
        return true;
    }
    
    if (event.mouseUp && !isOver) {
        setState(UIState::Normal);
        return true;
    }
    
    // Update hover state
    if (isOver) {
        setState(UIState::Hover);
    } else {
        setState(UIState::Normal);
    }
    
    return false;
}

} // namespace engine::ui
