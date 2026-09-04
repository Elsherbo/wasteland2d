#include "UIComponent.h"
#include "UIStyle.h"
#include "InputEvent.h"
#include "core/Logger.h"
#include <algorithm>

namespace engine::ui {

UIComponent::UIComponent() {
}

void UIComponent::setState(UIState newState) {
    if (state_ != newState) {
        UIState oldState = state_;
        state_ = newState;
        onStateChanged(oldState, newState);
        setDirty();
    }
}

void UIComponent::setVisible(bool visible) {
    setState(visible ? UIState::Normal : UIState::Hidden);
}

void UIComponent::update(double dt) {
    // Update children
    for (auto* child : children_) {
        if (child->isVisible()) {
            child->update(dt);
        }
    }
}

void UIComponent::render(Renderer& renderer) {
    // Render children
    for (auto* child : children_) {
        if (child->isVisible()) {
            child->render(renderer);
        }
    }
}

bool UIComponent::handleInput(const InputEventLegacy& event) {
    // Pass input to children (reverse order for top-most first)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->isVisible() && (*it)->isInteractable()) {
            if ((*it)->handleInput(event)) {
                return true;  // Input handled
            }
        }
    }
    return false;  // Input not handled
}

void UIComponent::layout() {
    // Calculate children positions
    for (auto* child : children_) {
        child->layout();
    }
}

glm::vec2 UIComponent::calculateMinSize() const {
    // Default implementation: return custom minimum size if set, otherwise current size
    if (customMinimumSize_.x > 0.0f || customMinimumSize_.y > 0.0f) {
        return customMinimumSize_;
    }
    return minSize_;
}

void UIComponent::guiInput(const InputEvent& event) {
    // Forward to children (reverse order for top-most first)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->isVisible() && (*it)->isInteractable()) {
            (*it)->guiInput(event);
            // If child accepted the event, stop propagating -- and mark
            // *this* container as having accepted it too. Without this,
            // acceptance only ever reaches whoever directly called
            // guiInput() on the accepting component: every ancestor above
            // that checks its *own* eventAccepted_ flag, which nothing
            // here ever set, so an ancestor's own accept-check (e.g.
            // UIScrollContainer::guiInput()'s "give nested content first
            // refusal" logic for mouse wheel) always reads false and the
            // event gets handled a second time further up the tree --
            // which is exactly why scrolling a nested scroll region was
            // also scrolling the outer one.
            if ((*it)->isEventAccepted()) {
                acceptEvent();
                return;
            }
        }
    }
}

void UIComponent::onStateChanged(UIState oldState, UIState newState) {
    // Override in subclasses for specific behavior
    (void)oldState;
    (void)newState;
}

bool UIComponent::containsPoint(glm::vec2 point) const {
    glm::vec2 worldPos = getWorldPosition();
    glm::vec2 worldSize = getWorldSize();
    
    return point.x >= worldPos.x && point.x <= worldPos.x + worldSize.x &&
           point.y >= worldPos.y && point.y <= worldPos.y + worldSize.y;
}

glm::vec2 UIComponent::getWorldPosition() const {
    glm::vec2 pos = position_;
    
    // Apply anchor offset
    glm::vec2 worldSize = getWorldSize();
    pos -= anchor_ * worldSize;
    
    // Apply parent transform
    if (parent_) {
        pos += parent_->getWorldPosition();
    }
    
    return pos;
}

glm::vec2 UIComponent::getWorldSize() const {
    glm::vec2 s = size_ * scale_;
    return s;
}

void UIComponent::addChild(std::unique_ptr<UIComponent> child) {
    child->parent_ = this;
    children_.push_back(child.get());
    childrenOwned_.push_back(std::move(child));
    setDirty();
    setLayoutDirty();  // Invalidate layout when children change
    invalidateParentLayout();  // Propagate up to parent containers
}

void UIComponent::removeChild(UIComponent* child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        child->parent_ = nullptr;
        children_.erase(it);
        
        // Remove from owned
        auto ownedIt = std::find_if(childrenOwned_.begin(), childrenOwned_.end(),
            [child](const std::unique_ptr<UIComponent>& ptr) { return ptr.get() == child; });
        if (ownedIt != childrenOwned_.end()) {
            childrenOwned_.erase(ownedIt);
        }
        
        setDirty();
    }
}

void UIComponent::requestPointerCapture() {
    // This would be implemented through UIManager in a real system
    // For now, it's a placeholder
    // TODO: Wire this to UIManager::setCapturedComponent
}

void UIComponent::releasePointerCapture() {
    // This would be implemented through UIManager in a real system
    // For now, it's a placeholder
    // TODO: Wire this to UIManager::clearCapture
}

void UIComponent::invalidateParentLayout() {
    // Propagate layout invalidation up the parent chain
    if (parent_) {
        parent_->setLayoutDirty();
        parent_->invalidateParentLayout();  // Recurse up
    }
}

void UIComponent::renderUI(UIRenderer& uiRenderer) {
    // Default implementation: render children
    for (auto* child : getChildren()) {
        child->renderUI(uiRenderer);
    }
}

} // namespace engine::ui
