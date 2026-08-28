#include "UIHBox.h"

namespace engine::ui {

UIHBox::UIHBox() {
}

void UIHBox::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    glm::vec2 contentArea = getContentArea();
    float currentX = padding_.x;
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        // Position child
        child->setPosition(glm::vec2(currentX, padding_.y));
        
        // Expand height if enabled
        if (expandChildren_) {
            child->setSize(glm::vec2(child->getSize().x, contentArea.y));
        }
        
        // Move to next position
        currentX += child->getSize().x + spacing_;
    }
    
    clearLayoutDirty();
    
    // Call layout on children
    UIContainer::layout();
}

} // namespace engine::ui
