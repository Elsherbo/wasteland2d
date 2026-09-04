#include "UIHBox.h"
#include <algorithm>

namespace engine::ui {

UIHBox::UIHBox() {
}

void UIHBox::layout() {
    if (!isLayoutDirty()) {
        return;
    }
    
    // If auto-sizing, resize this container to exactly fit its content
    // before measuring against it, instead of laying out into whatever
    // (possibly stale/hand-picked) size_ currently holds.
    if (getAutoSize()) {
        setSize(calculateMinSize());
    }
    
    glm::vec2 contentArea = getContentArea();
    
    // First pass: Measure children and calculate total desired size
    struct ChildInfo {
        UIComponent* child;
        glm::vec2 minSize;
        glm::vec2 desiredSize;
        SizeFlag flags;
    };
    
    std::vector<ChildInfo> childInfos;
    float totalDesiredWidth = 0.0f;
    float maxHeight = 0.0f;
    int expandCount = 0;
    
    for (auto* child : getChildren()) {
        if (!child->isVisible()) {
            continue;
        }
        
        glm::vec2 childMinSize = child->calculateMinSize();
        glm::vec2 childSize = child->getSize();
        SizeFlag flags = child->getSizeFlags();
        
        // Ensure child size is at least min size
        if (childSize.x < childMinSize.x) childSize.x = childMinSize.x;
        if (childSize.y < childMinSize.y) childSize.y = childMinSize.y;
        
        childInfos.push_back({child, childMinSize, childSize, flags});
        
        totalDesiredWidth += childSize.x;
        if (childSize.y > maxHeight) maxHeight = childSize.y;
        
        if ((static_cast<int>(flags) & static_cast<int>(SizeFlag::Expand))) {
            expandCount++;
        }
    }
    
    // Calculate available space for expansion (see UIVBox::layout() for why
    // spacing has to be subtracted here too, not just in calculateMinSize()).
    float totalSpacing = childInfos.size() > 1 ? spacing_ * static_cast<float>(childInfos.size() - 1) : 0.0f;
    float availableWidth = contentArea.x;
    float extraWidth = availableWidth - totalDesiredWidth - totalSpacing;
    
    // Second pass: Distribute extra space to expanding children
    for (auto& info : childInfos) {
        if ((static_cast<int>(info.flags) & static_cast<int>(SizeFlag::Expand)) && expandCount > 0 && extraWidth > 0) {
            float expansion = extraWidth / static_cast<float>(expandCount);
            info.desiredSize.x += expansion;
        }
        
        // Handle Fill flag for height
        if ((static_cast<int>(info.flags) & static_cast<int>(SizeFlag::Fill))) {
            info.desiredSize.y = contentArea.y;
        }
        
        // Apply size
        info.child->setSize(info.desiredSize);
        
        // Layout the child (recursively)
        info.child->layout();
    }
    
    // Third pass: Position children
    float currentX = padding_.x;
    for (auto& info : childInfos) {
        // Position child
        info.child->setPosition(glm::vec2(currentX, padding_.y));
        
        // Move to next position
        currentX += info.desiredSize.x + spacing_;
    }
    
    clearLayoutDirty();
}

glm::vec2 UIHBox::calculateMinSize() const {
    glm::vec2 minSize(0.0f, 0.0f);
    
    // Calculate total width as sum of children min widths + spacing
    float totalWidth = 0.0f;
    float maxHeight = 0.0f;
    
    const auto& children = getChildren();
    size_t visibleCount = 0;
    
    for (auto* child : children) {
        if (!child->isVisible()) {
            continue;
        }
        
        visibleCount++;
        
        // See UIVBox::calculateMinSize() for why this uses max(getSize(),
        // calculateMinSize()) instead of the raw minimum -- same mismatch,
        // same fix, transposed to the horizontal axis.
        glm::vec2 childMinSize = child->calculateMinSize();
        glm::vec2 childActual = child->getSize();
        float effectiveWidth = std::max(childMinSize.x, childActual.x);
        float effectiveHeight = std::max(childMinSize.y, childActual.y);
        
        totalWidth += effectiveWidth;
        if (effectiveHeight > maxHeight) maxHeight = effectiveHeight;
    }
    
    // Add spacing between children
    if (visibleCount > 1) {
        totalWidth += spacing_ * (visibleCount - 1);
    }
    
    minSize.x = totalWidth;
    minSize.y = maxHeight;
    
    // Add padding
    minSize += padding_ * 2.0f;
    
    // Respect custom minimum size if set
    if (customMinimumSize_.x > 0.0f) minSize.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) minSize.y = customMinimumSize_.y;
    
    return minSize;
}

} // namespace engine::ui
