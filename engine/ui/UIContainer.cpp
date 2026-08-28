#include "UIContainer.h"

namespace engine::ui {

UIContainer::UIContainer() {
    setInteractable(false);  // Containers are not interactive by default
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

} // namespace engine::ui
