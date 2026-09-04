#include "UIVBox.h"
#include <algorithm>

namespace engine::ui {

UIVBox::UIVBox() {
}

void UIVBox::layout() {
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
    float totalDesiredHeight = 0.0f;
    float maxWidth = 0.0f;
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
        
        totalDesiredHeight += childSize.y;
        if (childSize.x > maxWidth) maxWidth = childSize.x;
        
        if (static_cast<int>(flags) & static_cast<int>(SizeFlag::Expand)) {
            expandCount++;
        }
    }
    
    // Calculate available space for expansion. Spacing between children is
    // consumed space just like their own height -- calculateMinSize() already
    // accounts for it (spacing_ * (visibleCount - 1)), so this must too, or
    // extraHeight is overestimated and the positioning pass below (which does
    // add spacing_ between children) pushes later children past the bottom
    // of the container by spacing_ * (n - 1).
    float totalSpacing = childInfos.size() > 1 ? spacing_ * static_cast<float>(childInfos.size() - 1) : 0.0f;
    float availableHeight = contentArea.y;
    float extraHeight = availableHeight - totalDesiredHeight - totalSpacing;
    
    // Second pass: Distribute extra space to expanding children
    for (auto& info : childInfos) {
        if ((static_cast<int>(info.flags) & static_cast<int>(SizeFlag::Expand)) && expandCount > 0 && extraHeight > 0) {
            float expansion = extraHeight / static_cast<float>(expandCount);
            info.desiredSize.y += expansion;
        }
        
        // Handle Fill flag for width
        if ((static_cast<int>(info.flags) & static_cast<int>(SizeFlag::Fill))) {
            info.desiredSize.x = contentArea.x;
        }
        
        // Apply size
        info.child->setSize(info.desiredSize);
        
        // Layout the child (recursively)
        info.child->layout();
    }
    
    // Third pass: Position children
    float currentY = padding_.y;
    for (auto& info : childInfos) {
        // Position child
        info.child->setPosition(glm::vec2(padding_.x, currentY));
        
        // Move to next position
        currentY += info.desiredSize.y + spacing_;
    }
    
    clearLayoutDirty();
}

glm::vec2 UIVBox::calculateMinSize() const {
    glm::vec2 minSize(0.0f, 0.0f);
    
    // Calculate total height as sum of children min heights + spacing
    float totalHeight = 0.0f;
    float maxWidth = 0.0f;
    
    const auto& children = getChildren();
    size_t visibleCount = 0;
    
    for (auto* child : children) {
        if (!child->isVisible()) {
            continue;
        }
        
        visibleCount++;
        
        // A child's *effective* size for measurement is whatever its own
        // layout() will actually give it -- its explicitly-set size where
        // that's larger than its reported minimum, not the bare minimum
        // alone. layout()'s own first pass already does exactly this
        // (childSize = max(getSize(), calculateMinSize())) before placing
        // children; summing only the raw calculateMinSize() here instead
        // meant this container could report itself smaller than what it's
        // about to actually lay its children out to. That's precisely
        // what let a 30px-tall slider row (whose own calculateMinSize()
        // is only ~20px) push past the bottom of a section card that had
        // been sized using the smaller, wrong figure.
        glm::vec2 childMinSize = child->calculateMinSize();
        glm::vec2 childActual = child->getSize();
        float effectiveHeight = std::max(childMinSize.y, childActual.y);
        float effectiveWidth = std::max(childMinSize.x, childActual.x);
        
        totalHeight += effectiveHeight;
        if (effectiveWidth > maxWidth) maxWidth = effectiveWidth;
    }
    
    // Add spacing between children
    if (visibleCount > 1) {
        totalHeight += spacing_ * (visibleCount - 1);
    }
    
    minSize.x = maxWidth;
    minSize.y = totalHeight;
    
    // Add padding
    minSize += padding_ * 2.0f;
    
    // Respect custom minimum size if set
    if (customMinimumSize_.x > 0.0f) minSize.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) minSize.y = customMinimumSize_.y;
    
    return minSize;
}

} // namespace engine::ui
