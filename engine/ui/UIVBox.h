#pragma once

#include "UIContainer.h"

namespace engine::ui {

// UIVBox - vertical box layout (like Godot VBoxContainer)
class UIVBox : public UIContainer {
public:
    UIVBox();
    ~UIVBox() override = default;
    
    // Spacing between children
    float getSpacing() const { return spacing_; }
    void setSpacing(float spacing) { spacing_ = spacing; setLayoutDirty(); }
    
    // Expand children to fill width
    bool shouldExpandChildren() const { return expandChildren_; }
    void setExpandChildren(bool expand) { expandChildren_ = expand; setLayoutDirty(); }
    
    // Layout
    void layout() override;

private:
    float spacing_ = 0.0f;
    bool expandChildren_ = false;
};

} // namespace engine::ui
