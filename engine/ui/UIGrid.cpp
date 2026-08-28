#include "UIGrid.h"
#include <glm/vec2.hpp>

namespace engine::ui {

UIGrid::UIGrid() {
}

void UIGrid::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    int childIndex = 0;
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        // Calculate grid position
        int col = childIndex % columns_;
        int row = childIndex / columns_;
        
        // Calculate position
        float x = padding_.x + col * (cellWidth_ + spacing_);
        float y = padding_.y + row * (cellHeight_ + spacing_);
        
        // Position and size child
        child->setPosition(glm::vec2(x, y));
        child->setSize(glm::vec2(cellWidth_, cellHeight_));
        
        childIndex++;
    }
    
    clearLayoutDirty();
    
    // Call layout on children
    UIContainer::layout();
}

} // namespace engine::ui
