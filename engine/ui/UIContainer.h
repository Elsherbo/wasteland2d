#pragma once

#include "UIComponent.h"
#include <glm/vec2.hpp>

namespace engine::ui {

// UIContainer - base class for layout containers
class UIContainer : public UIComponent {
public:
    UIContainer();
    ~UIContainer() override = default;
    
    // Layout management
    virtual void layout() override;
    
    // Content area (excluding padding)
    glm::vec2 getContentArea() const;
    
    // Padding
    const glm::vec2& getPadding() const { return padding_; }
    void setPadding(const glm::vec2& padding) { padding_ = padding; setDirty(); }
    
    // Layout dirty flag
    bool isLayoutDirty() const { return layoutDirty_; }
    void setLayoutDirty() { layoutDirty_ = true; }
    void clearLayoutDirty() { layoutDirty_ = false; }

protected:
    glm::vec2 padding_ = glm::vec2(0.0f);
    bool layoutDirty_ = true;
};

} // namespace engine::ui
