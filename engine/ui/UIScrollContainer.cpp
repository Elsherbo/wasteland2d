#include "UIScrollContainer.h"
#include "InputEvent.h"
#include "UIRenderer.h"

namespace engine::ui {

UIScrollContainer::UIScrollContainer() {
}

glm::vec2 UIScrollContainer::calculateMinSize() const {
    // A scroll viewport's natural footprint is its own configured window
    // size, not its (usually much larger -- that's the point) content.
    // UIContainer's inherited default does "max of children," which for a
    // ScrollContainer means its reported min size grows with whatever's
    // scrolled inside it -- the opposite of what a parent VBox/HBox needs
    // to correctly reserve space for it as a fixed-size viewport.
    if (customMinimumSize_.x > 0.0f || customMinimumSize_.y > 0.0f) {
        return customMinimumSize_;
    }
    return size_;
}

void UIScrollContainer::clampScroll() {
    glm::vec2 worldSize = getWorldSize();
    
    // maxScroll has to account for padding_ on both ends: layout() starts
    // the content at (padding_ - scrollPosition_), so the content's true
    // bottom/right edge sits at (padding_ + contentSize_) before any
    // scroll offset -- not just contentSize_ alone. Without the padding_
    // term here, the computed "maximum" scroll position stops short of
    // that true edge by padding_ on each axis, so the last bit of content
    // (exactly padding_'s worth) can never be scrolled into view no matter
    // how far the user scrolls -- which is what made the very bottom of
    // the settings panel permanently unreachable even at "max scroll."
    if (horizontalScroll_) {
        float maxScrollX = (contentSize_.x + padding_.x * 2.0f) - worldSize.x;
        if (maxScrollX < 0.0f) maxScrollX = 0.0f;
        if (scrollPosition_.x < 0.0f) scrollPosition_.x = 0.0f;
        if (scrollPosition_.x > maxScrollX) scrollPosition_.x = maxScrollX;
    } else {
        scrollPosition_.x = 0.0f;
    }
    
    if (verticalScroll_) {
        float maxScrollY = (contentSize_.y + padding_.y * 2.0f) - worldSize.y;
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
    
    // Auto-derive scrollable content size from a single content child
    // instead of requiring a manual setContentSize() call that can drift
    // out of sync (Godot's ScrollContainer does this automatically using
    // the child's minimum size). Multi-child scroll containers still need
    // an explicit setContentSize() call since there's no single child to
    // measure.
    const auto& scrollChildren = getChildren();
    if (scrollChildren.size() == 1) {
        contentSize_ = scrollChildren[0]->calculateMinSize();
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
            if (verticalScroll_ && contentSize_.y > 0.0f) {
                float scrollbarHeight = worldSize.y;
                float maxScroll = (contentSize_.y + padding_.y * 2.0f) - worldSize.y;
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
    
    // Handle mouse wheel for vertical scrolling. This block always returns
    // (never falls through to the generic forwarding at the bottom of this
    // function) so that "not over this container" and "nothing nested
    // claimed it" both correctly result in the event going unhandled
    // instead of being dispatched to children a second time.
    if (event.isMouseWheel()) {
        const auto* wheelData = event.getMouseWheelData();
        
        // Only claim wheel events that actually land over this container --
        // UIManager::dispatchInput() doesn't bounds-check wheel events
        // itself, so without this a ScrollContainer would consume every
        // wheel tick anywhere in the window, not just ones over it.
        if (!wheelData || !containsPoint(wheelData->position)) {
            return;
        }
        
        // Give nested content first refusal -- if a scrollable region is
        // nested inside this one (e.g. a small scroll list inside the main
        // panel) and the cursor is over that inner region, it should scroll
        // instead of this outer container intercepting every wheel tick
        // before children ever see it.
        UIContainer::guiInput(event);
        if (isEventAccepted()) {
            return;
        }
        
        if (verticalScroll_) {
            // wheelData->delta is positive when the wheel is scrolled away
            // from the user (physically "up") -- that gesture should move
            // the viewport toward the *top* of the content, i.e. decrease
            // scrollPosition_.y, not increase it. The previous `+=` here
            // had it backwards: scrolling up increased scrollPosition_.y,
            // which shifts content up on screen and reveals what's further
            // *down* the panel -- exactly the inverted behavior reported.
            // Also bumped from 10px/tick to a snappier default -- 10px took
            // dozens of notches to cross a full-height panel.
            constexpr float kWheelScrollSpeed = 60.0f;  // px per wheel tick
            float scrollAmount = wheelData->delta * kWheelScrollSpeed;
            scrollPosition_.y -= scrollAmount;
            clampScroll();
            setLayoutDirty();  // Re-layout children with new scroll position
            acceptEvent();
        }
        return;
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
    
    if (!verticalScroll_ || (contentSize_.y + padding_.y * 2.0f) <= worldSize.y) {
        return false;
    }
    
    float scrollbarWidth = 12.0f;
    float scrollbarX = worldPos.x + worldSize.x - scrollbarWidth;
    float scrollbarHeight = worldSize.y;
    
    // Calculate thumb position and size
    float maxScroll = (contentSize_.y + padding_.y * 2.0f) - worldSize.y;
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
    
    if (!verticalScroll_ || (contentSize_.y + padding_.y * 2.0f) <= worldSize.y) {
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
