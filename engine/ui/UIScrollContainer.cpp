#include "UIScrollContainer.h"

namespace engine::ui {

UIScrollContainer::UIScrollContainer() {
}

void UIScrollContainer::clampScroll() {
    glm::vec2 contentArea = getContentArea();
    
    if (horizontalScroll_) {
        float maxScrollX = contentSize_.x - contentArea.x;
        if (maxScrollX < 0.0f) maxScrollX = 0.0f;
        if (scrollPosition_.x < 0.0f) scrollPosition_.x = 0.0f;
        if (scrollPosition_.x > maxScrollX) scrollPosition_.x = maxScrollX;
    } else {
        scrollPosition_.x = 0.0f;
    }
    
    if (verticalScroll_) {
        float maxScrollY = contentSize_.y - contentArea.y;
        if (maxScrollY < 0.0f) maxScrollY = 0.0f;
        if (scrollPosition_.y < 0.0f) scrollPosition_.y = 0.0f;
        if (scrollPosition_.y > maxScrollY) scrollPosition_.y = maxScrollY;
    } else {
        scrollPosition_.y = 0.0f;
    }
}

void UIScrollContainer::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    // Position children based on scroll offset
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        glm::vec2 childPos = child->getPosition();
        childPos -= scrollPosition_;
        child->setPosition(childPos);
    }
    
    clearLayoutDirty();
    
    // Call layout on children
    UIContainer::layout();
}

void UIScrollContainer::update(double dt) {
    // TODO: Implement scroll input handling in Phase 6
    // For now, this is a placeholder
    (void)dt;
    
    // Pseudo-code:
    // 1. Handle mouse wheel input
    // 2. Update scroll position
    // 3. Clamp scroll position
    // 4. Re-layout children
    
    UIContainer::update(dt);
}

} // namespace engine::ui
