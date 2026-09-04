#pragma once

#include "UIComponent.h"
#include <glm/vec2.hpp>
#include <string>

namespace engine::ui {

// UIContainer - base class for layout containers
class UIContainer : public UIComponent {
public:
    UIContainer();
    ~UIContainer() override = default;
    
    // Layout management
    virtual void layout() override;
    virtual glm::vec2 calculateMinSize() const override;
    
    // UI rendering
    virtual void renderUI(UIRenderer& uiRenderer) override;
    
    // Content area (excluding padding)
    glm::vec2 getContentArea() const;
    
    // Padding
    const glm::vec2& getPadding() const { return padding_; }
    void setPadding(const glm::vec2& padding) { padding_ = padding; setLayoutDirty(); invalidateParentLayout(); }
    
    // Layout dirty flag
    bool isLayoutDirty() const { return layoutDirty_; }
    void setLayoutDirty() {
        layoutDirty_ = true;
        // Every OTHER dirtying setter on this class/UIComponent (setSize,
        // setPosition, setPadding, ...) also calls invalidateParentLayout()
        // to walk up and mark ancestors dirty too -- this one didn't. Every
        // container's own layout() starts with "if (!isLayoutDirty())
        // return", which blocks recursion into children entirely, not just
        // skipping its own repositioning math. So a component marked dirty
        // only on itself (e.g. UIScrollContainer's wheel/drag handlers
        // calling setLayoutDirty() directly) would never actually get its
        // layout() reached on the next pass unless something else had
        // independently dirtied an ancestor too -- exactly what let a
        // nested scroll list's scrollPosition_ update (and its scrollbar,
        // which reads that directly) while its content, only repositioned
        // inside layout(), silently never moved.
        invalidateParentLayout();
    }
    void clearLayoutDirty() { layoutDirty_ = false; }
    
    // Auto-size: when true, the container resizes itself to exactly fit its
    // content (calculateMinSize()) at the start of every layout() pass instead
    // of requiring a hand-picked fixed size. Root/standalone containers (ones
    // not sized by a parent VBox/HBox) should turn this on so their declared
    // size can never drift out of sync with their actual content.
    bool getAutoSize() const { return autoSize_; }
    void setAutoSize(bool autoSize) { autoSize_ = autoSize; setLayoutDirty(); }
    
    // Style name used to look up this container's visual style in the theme
    // (see UIThemeManager). Defaults to "Container" for backward
    // compatibility; set to something like "Panel" or "Section" to give
    // different roles a distinct look instead of every nested box rendering
    // identically.
    const std::string& getStyleName() const { return styleName_; }
    void setStyleName(const std::string& styleName) { styleName_ = styleName; }

protected:
    glm::vec2 padding_ = glm::vec2(0.0f);
    bool layoutDirty_ = true;
    bool autoSize_ = false;
    std::string styleName_ = "Container";
};

} // namespace engine::ui
