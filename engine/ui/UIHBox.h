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
    
    // Layout
    void layout() override;
    glm::vec2 calculateMinSize() const override;

private:
    float spacing_ = 0.0f;
};

} // namespace engine::ui
