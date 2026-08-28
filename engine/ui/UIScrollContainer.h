#pragma once

#include "UIContainer.h"
#include <glm/vec2.hpp>

namespace engine::ui {

// UIScrollContainer - scrollable content area
class UIScrollContainer : public UIContainer {
public:
    UIScrollContainer();
    ~UIScrollContainer() override = default;
    
    // Content size
    const glm::vec2& getContentSize() const { return contentSize_; }
    void setContentSize(const glm::vec2& size) { contentSize_ = size; setLayoutDirty(); }
    
    // Scroll position
    const glm::vec2& getScrollPosition() const { return scrollPosition_; }
    void setScrollPosition(const glm::vec2& pos) { scrollPosition_ = pos; clampScroll(); setDirty(); }
    
    // Scroll enable
    bool isHorizontalScrollEnabled() const { return horizontalScroll_; }
    void setHorizontalScrollEnabled(bool enabled) { horizontalScroll_ = enabled; }
    
    bool isVerticalScrollEnabled() const { return verticalScroll_; }
    void setVerticalScrollEnabled(bool enabled) { verticalScroll_ = enabled; }
    
    // Layout
    void layout() override;
    
    // Update (for scroll input)
    void update(double dt) override;

private:
    void clampScroll();
    
    glm::vec2 contentSize_ = glm::vec2(100.0f, 100.0f);
    glm::vec2 scrollPosition_ = glm::vec2(0.0f);
    bool horizontalScroll_ = false;
    bool verticalScroll_ = true;
};

} // namespace engine::ui
