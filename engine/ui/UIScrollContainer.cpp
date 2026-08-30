#include "UIScrollContainer.h"
#include "InputEvent.h"
#include "UIRenderer.h"

namespace engine::ui {

UIScrollContainer::UIScrollContainer() {
}

void UIScrollContainer::clampScroll() {
    glm::vec2 worldSize = getWorldSize();
    
    if (horizontalScroll_) {
        float maxScrollX = contentSize_.x - worldSize.x;
        if (maxScrollX < 0.0f) maxScrollX = 0.0f;
        if (scrollPosition_.x < 0.0f) scrollPosition_.x = 0.0f;
        if (scrollPosition_.x > maxScrollX) scrollPosition_.x = maxScrollX;
    } else {
        scrollPosition_.x = 0.0f;
    }
    
    if (verticalScroll_) {
        float maxScrollY = contentSize_.y - worldSize.y;
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
    
    glm::vec2 contentArea = getContentArea();
    
    // Position children with scroll offset applied
    float currentX = padding_.x - scrollPosition_.x;
    float currentY = padding_.y - scrollPosition_.y;
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        // Position child at (currentX, currentY)
        child->setPosition(glm::vec2(currentX, currentY));
        
        // Layout the child
        child->layout();
        
        // Move to next position (vertical stacking by default)
        // TODO: Support horizontal layout modes
        currentY += child->getSize().y + 5.0f;  // 5px spacing
    }
    
    clearLayoutDirty();
}

void UIScrollContainer::update(double dt) {
    // Scroll input handling is now in guiInput
    (void)dt;
    UIContainer::update(dt);
}

void UIScrollContainer::guiInput(const InputEvent& event) {
    if (!isInteractable() || getState() == UIState::Disabled) {
        return;
    }
    
    const auto* mouseData = event.getMouseData();
    glm::vec2 worldPos = getWorldPosition();
    glm::vec2 worldSize = getWorldSize();
    
    // Handle thumb dragging
    if (draggingThumb_ && mouseData) {
        if (event.isMouseMotion()) {
            // Calculate scroll based on drag
            if (verticalScroll_) {
                float scrollbarHeight = worldSize.y;
                float maxScroll = contentSize_.y - worldSize.y;
                float thumbRatio = worldSize.y / contentSize_.y;
                float thumbHeight = scrollbarHeight * thumbRatio;
                float thumbRange = scrollbarHeight - thumbHeight;
                
                if (thumbRange > 0) {
                    float deltaY = mouseData->position.y - dragStartPos_.y;
                    float deltaScroll = (deltaY / thumbRange) * maxScroll;
                    scrollPosition_.y = dragStartScroll_.y + deltaScroll;
                    clampScroll();
                    setLayoutDirty();
                }
            }
            acceptEvent();
            return;
        } else if (event.isMouseButton() && !mouseData->pressed) {
            // Release drag
            draggingThumb_ = false;
            acceptEvent();
            return;
        }
    }
    
    // Handle mouse wheel for vertical scrolling
    if (event.isMouseWheel()) {
        const auto* wheelData = event.getMouseWheelData();
        if (wheelData && verticalScroll_) {
            // Scroll based on wheel delta (positive = scroll down)
            float scrollAmount = wheelData->delta * 10.0f;  // 10px per wheel tick
            scrollPosition_.y += scrollAmount;
            clampScroll();
            setLayoutDirty();  // Re-layout children with new scroll position
            acceptEvent();
            return;
        }
    }
    
    // Handle mouse button for thumb dragging
    if (event.isMouseButton() && mouseData && mouseData->pressed) {
        if (isPointInScrollbarThumb(mouseData->position)) {
            draggingThumb_ = true;
            dragStartPos_ = mouseData->position;
            dragStartScroll_ = scrollPosition_;
            acceptEvent();
            return;
        }
    }
    
    // Input clipping: Check if point is within viewport before passing to children
    if (mouseData) {
        // If mouse is outside viewport, don't pass to children
        if (mouseData->position.x < worldPos.x || mouseData->position.x > worldPos.x + worldSize.x ||
            mouseData->position.y < worldPos.y || mouseData->position.y > worldPos.y + worldSize.y) {
            // Still accept the event for the container itself (e.g., scrollbar), but don't pass to children
            return;
        }
    }
    
    // Pass to base for other input
    UIContainer::guiInput(event);
}

bool UIScrollContainer::isPointInScrollbarThumb(glm::vec2 point) const {
    glm::vec2 worldPos = getWorldPosition();
    glm::vec2 worldSize = getWorldSize();
    
    if (!verticalScroll_ || contentSize_.y <= worldSize.y) {
        return false;
    }
    
    float scrollbarWidth = 12.0f;
    float scrollbarX = worldPos.x + worldSize.x - scrollbarWidth;
    float scrollbarHeight = worldSize.y;
    
    // Calculate thumb position and size
    float maxScroll = contentSize_.y - worldSize.y;
    float scrollRatio = (maxScroll > 0) ? scrollPosition_.y / maxScroll : 0;
    float thumbRatio = worldSize.y / contentSize_.y;
    float thumbHeight = scrollbarHeight * thumbRatio;
    float thumbY = worldPos.y + (scrollbarHeight - thumbHeight) * scrollRatio;
    
    // Check if point is in thumb
    return point.x >= scrollbarX && point.x <= scrollbarX + scrollbarWidth &&
           point.y >= thumbY && point.y <= thumbY + thumbHeight;
}

bool UIScrollContainer::isPointInScrollbarTrack(glm::vec2 point) const {
    glm::vec2 worldPos = getWorldPosition();
    glm::vec2 worldSize = getWorldSize();
    
    if (!verticalScroll_ || contentSize_.y <= worldSize.y) {
        return false;
    }
    
    float scrollbarWidth = 12.0f;
    float scrollbarX = worldPos.x + worldSize.x - scrollbarWidth;
    float scrollbarHeight = worldSize.y;
    
    // Check if point is in track
    return point.x >= scrollbarX && point.x <= scrollbarX + scrollbarWidth &&
           point.y >= worldPos.y && point.y <= worldPos.y + scrollbarHeight;
}

void UIScrollContainer::renderUI(UIRenderer& uiRenderer) {
    // Render children (clipped by UIRenderer's container handling)
    UIContainer::renderUI(uiRenderer);
    
    // TODO: Render scrollbar here
    // This would call back to UIRenderer helper methods for scrollbar rendering
    (void)uiRenderer;
}

} // namespace engine::ui
