#include "UIComponent.h"
#include "UIStyle.h"
#include "core/Logger.h"

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

void UIComponent::guiInput(const InputEvent& event) {
    // Base implementation - can be overridden in subclasses
    (void)event;
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

} // namespace engine::ui
