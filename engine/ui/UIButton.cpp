#include "UIButton.h"

namespace engine::ui {

UIButton::UIButton() {
    setInteractable(true);
}

void UIButton::setToggled(bool toggled) {
    if (toggleMode_) {
        toggled_ = toggled;
        setState(toggled_ ? UIState::Active : UIState::Normal);
        setDirty();
    }
}

void UIButton::render(Renderer& renderer) {
    // TODO: Implement rendering in Phase 6 with actual Renderer
    // For now, this is a placeholder
    (void)renderer;
    
    // Pseudo-code for rendering:
    // 1. Get style for current state
    // 2. If image exists for state, render image
    // 3. Otherwise, render background color from style
    // 4. Render border if borderWidth > 0
    // 5. Render text if not empty
    // 6. Apply shadow if enabled
}

void UIButton::guiInput(const InputEvent& event) {
    if (!isInteractable() || getState() == UIState::Disabled) {
        return;
    }
    
    if (!event.isMouseButton()) return;
    
    const auto* mouseData = event.getMouseData();
    if (!mouseData) return;
    
    bool isOver = containsPoint(mouseData->position);
    
    if (mouseData->pressed && isOver) {
        if (toggleMode_) {
            setToggled(!toggled_);
        } else {
            setState(UIState::Active);
        }
        wasPressed_ = true;
        acceptEvent();
        return;
    }
    
    if (!mouseData->pressed && wasPressed_) {
        wasPressed_ = false;
        
        if (isOver) {
            if (!toggleMode_) {
                setState(UIState::Hover);
            }
            
            // Trigger click callback
            if (onClick) {
                onClick();
            }
            acceptEvent();
            return;
        } else {
            if (!toggleMode_) {
                setState(UIState::Normal);
            }
        }
    }
    
    // Update hover state
    if (isOver && !wasPressed_ && getState() != UIState::Active) {
        setState(UIState::Hover);
    } else if (!isOver && !wasPressed_ && getState() != UIState::Active) {
        if (!toggleMode_ || !toggled_) {
            setState(UIState::Normal);
        }
    }
}

void UIButton::onStateChanged(UIState oldState, UIState newState) {
    // Handle state transitions
    (void)oldState;
    (void)newState;
    
    // TODO: Play sounds, animations, etc. in Phase 6
}

} // namespace engine::ui
