#pragma once

#include "UIComponent.h"
#include <string>

namespace engine::ui {

// UILabel - text display component
class UILabel : public UIComponent {
public:
    UILabel();
    ~UILabel() override = default;
    
    // Text
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; setDirty(); }
    
    // Word wrap
    bool isWordWrap() const { return wordWrap_; }
    void setWordWrap(bool wrap) { wordWrap_ = wrap; setDirty(); }
    
    // Alignment
    TextAlignment getAlignment() const { return alignment_; }
    void setAlignment(TextAlignment alignment) { alignment_ = alignment; setDirty(); }
    
    // Heading: renders with the theme's heading font/size and the
    // "Label.Heading" style instead of "Label" -- for section titles that
    // need to visually outrank the body text beneath them, without
    // needing a whole separate component type.
    bool isHeading() const { return heading_; }
    void setHeading(bool heading) { heading_ = heading; setDirty(); }
    
    // Lifecycle
    void render(Renderer& renderer) override;
    void renderUI(UIRenderer& uiRenderer) override;
    glm::vec2 calculateMinSize() const override;

private:
    std::string text_;
    bool wordWrap_ = false;
    TextAlignment alignment_ = TextAlignment::Left;
    bool heading_ = false;
};

} // namespace engine::ui