#pragma once

#include "UIContainer.h"

namespace engine::ui {

// UIHBox - horizontal box layout (like Godot HBoxContainer)
class UIHBox : public UIContainer {
public:
    UIHBox();
    ~UIHBox() override = default;
    
    // Spacing between children
    float getSpacing() const { return spacing_; }
    void setSpacing(float spacing) { spacing_ = spacing; setLayoutDirty(); }
    
    // Expand children to fill height
    bool shouldExpandChildren() const { return expandChildren_; }
    void setExpandChildren(bool expand) { expandChildren_ = expand; setLayoutDirty(); }
    
    // Layout
    void layout() override;

private:
    float spacing_ = 0.0f;
    bool expandChildren_ = false;
};

} // namespace engine::ui
