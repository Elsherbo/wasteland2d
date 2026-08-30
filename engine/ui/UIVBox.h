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
    
    // Layout
    void layout() override;
    glm::vec2 calculateMinSize() const override;

private:
    float spacing_ = 0.0f;
};

} // namespace engine::ui
