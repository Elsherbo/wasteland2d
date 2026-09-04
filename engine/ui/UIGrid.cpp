#include "UIGrid.h"
#include <glm/vec2.hpp>

namespace engine::ui {

UIGrid::UIGrid() {
}

void UIGrid::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    // columns_ <= 0 would make "childIndex % columns_" / "childIndex / columns_"
    // below divide/modulo by zero (UB). setColumns() has no lower-bound check,
    // so guard here instead of trusting every caller.
    int columns = columns_ > 0 ? columns_ : 1;
    
    int childIndex = 0;
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        // Calculate grid position
        int col = childIndex % columns;
        int row = childIndex / columns;
        
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

glm::vec2 UIGrid::calculateMinSize() const {
    // UIContainer's default (max child size) has nothing to do with a
    // grid's actual footprint -- it under-reports height to whatever
    // parent is measuring this grid (a VBox stacking it above more
    // content, a ScrollContainer sizing its scroll range from it, etc.),
    // which is exactly what let content below a UIGrid become unreachable
    // by scroll. Compute rows from the *visible child count*, not the
    // rows_ field: layout() above never actually caps placement at
    // rows_, it just wraps at columns_, so rows_ can silently drift out
    // of sync with what's really placed -- the child count is ground
    // truth, rows_ isn't.
    int columns = columns_ > 0 ? columns_ : 1;
    
    int visibleCount = 0;
    for (auto* child : getChildren()) {
        if (child->isVisible()) {
            visibleCount++;
        }
    }
    
    int rowsNeeded = 0;
    if (visibleCount > 0) {
        rowsNeeded = (visibleCount + columns - 1) / columns;  // ceil division
    }
    
    glm::vec2 minSize(0.0f, 0.0f);
    if (rowsNeeded > 0) {
        minSize.x = static_cast<float>(columns) * cellWidth_ + static_cast<float>(columns - 1) * spacing_;
        minSize.y = static_cast<float>(rowsNeeded) * cellHeight_ + static_cast<float>(rowsNeeded - 1) * spacing_;
    }
    
    minSize += padding_ * 2.0f;
    
    if (customMinimumSize_.x > 0.0f) minSize.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) minSize.y = customMinimumSize_.y;
    
    return minSize;
}

} // namespace engine::ui
