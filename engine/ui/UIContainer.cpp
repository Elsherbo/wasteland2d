#include "UIContainer.h"
#include "UIRenderer.h"

namespace engine::ui {

UIContainer::UIContainer() {
    setInteractable(true);  // Containers are interactive by default to pass events to children
}

void UIContainer::layout() {
    // Base implementation - just clear dirty flag
    // Subclasses should override with specific layout logic
    layoutDirty_ = false;
    
    // Call layout on children
    for (auto* child : getChildren()) {
        child->layout();
    }
}

glm::vec2 UIContainer::getContentArea() const {
    glm::vec2 worldSize = getWorldSize();
    return worldSize - padding_ * 2.0f;
}

glm::vec2 UIContainer::calculateMinSize() const {
    // Calculate min size based on children
    glm::vec2 minSize(0.0f, 0.0f);
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        glm::vec2 childMinSize = child->calculateMinSize();
        
        // For now, just use the maximum child size (containers will override)
        if (childMinSize.x > minSize.x) minSize.x = childMinSize.x;
        if (childMinSize.y > minSize.y) minSize.y = childMinSize.y;
    }
    
    // Add padding
    minSize += padding_ * 2.0f;
    
    // Respect custom minimum size if set
    if (customMinimumSize_.x > 0.0f) minSize.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) minSize.y = customMinimumSize_.y;
    
    return minSize;
}

void UIContainer::renderUI(UIRenderer& uiRenderer) {
    uiRenderer.renderContainerHelper(this);
}

} // namespace engine::ui
