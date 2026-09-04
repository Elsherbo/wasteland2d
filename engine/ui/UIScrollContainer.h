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
    glm::vec2 calculateMinSize() const override;
    
    // Update (for scroll input)
    void update(double dt) override;
    
    // GUI Input (for mouse wheel scrolling and thumb dragging)
    void guiInput(const InputEvent& event) override;
    
    // UI rendering (for scrollbar)
    void renderUI(UIRenderer& uiRenderer) override;

private:
    void clampScroll();
    bool isPointInScrollbarThumb(glm::vec2 point) const;
    bool isPointInScrollbarTrack(glm::vec2 point) const;
    
    glm::vec2 contentSize_ = glm::vec2(0.0f, 0.0f);  // Zero means auto-derive from child
    glm::vec2 scrollPosition_ = glm::vec2(0.0f);
    bool horizontalScroll_ = false;
    bool verticalScroll_ = true;
    
    // Thumb dragging state
    bool draggingThumb_ = false;
    glm::vec2 dragStartPos_ = glm::vec2(0.0f);
    glm::vec2 dragStartScroll_ = glm::vec2(0.0f);
};

} // namespace engine::ui
