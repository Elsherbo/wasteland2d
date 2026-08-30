#include "UIButton.h"
#include "InputEvent.h"
#include "UIRenderer.h"
#include <glm/vec2.hpp>

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

void UIButton::renderUI(UIRenderer& uiRenderer) {
    uiRenderer.renderButtonHelper(this);
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
    
    const auto* mouseData = event.getMouseData();
    if (!mouseData) return;
    
    bool isOver = containsPoint(mouseData->position);
    
    // Handle mouse motion for hover state
    if (event.isMouseMotion()) {
        if (wasPressed_) {
            // While pressed, stay in Active if over, go to Normal if not over
            if (isOver) {
                setState(UIState::Active);
            } else {
                if (!toggleMode_ || !toggled_) {
                    setState(UIState::Normal);
                }
            }
        } else {
            // Not pressed, handle normal hover
            if (isOver && getState() != UIState::Active) {
                setState(UIState::Hover);
            } else if (!isOver && getState() != UIState::Active) {
                if (!toggleMode_ || !toggled_) {
                    setState(UIState::Normal);
                }
            }
        }
        return;
    }
    
    // Handle mouse button events
    if (!event.isMouseButton()) return;
    
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
            
            // Trigger click callback on mouse up
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
}

void UIButton::onStateChanged(UIState oldState, UIState newState) {
    // Handle state transitions
    (void)oldState;
    (void)newState;
    
    // TODO: Play sounds, animations, etc. in Phase 6
}

glm::vec2 UIButton::calculateMinSize() const {
    // Calculate minimum size based on text length (similar to label)
    float estimatedWidth = static_cast<float>(text_.length() * 10) + 20.0f;  // +20px padding
    float estimatedHeight = 30.0f;  // Default button height
    
    // Respect custom minimum size if set
    glm::vec2 calculatedMin(estimatedWidth, estimatedHeight);
    if (customMinimumSize_.x > 0.0f) calculatedMin.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) calculatedMin.y = customMinimumSize_.y;
    
    // Ensure minimum size
    if (calculatedMin.x < 80.0f) calculatedMin.x = 80.0f;
    if (calculatedMin.y < 30.0f) calculatedMin.y = 30.0f;
    
    return calculatedMin;
}

} // namespace engine::ui
