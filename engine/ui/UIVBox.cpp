#include "UIVBox.h"

namespace engine::ui {

UIVBox::UIVBox() {
}

void UIVBox::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    glm::vec2 contentArea = getContentArea();
    float currentY = padding_.y;
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        // Position child
        child->setPosition(glm::vec2(padding_.x, currentY));
        
        // Expand width if enabled
        if (expandChildren_) {
            child->setSize(glm::vec2(contentArea.x, child->getSize().y));
        }
        
        // Move to next position
        currentY += child->getSize().y + spacing_;
    }
    
    clearLayoutDirty();
    
    // Call layout on children
    UIContainer::layout();
}

} // namespace engine::ui
