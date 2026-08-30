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
    
    // Lifecycle
    void render(Renderer& renderer) override;
    void renderUI(UIRenderer& uiRenderer) override;
    glm::vec2 calculateMinSize() const override;

private:
    std::string text_;
    bool wordWrap_ = false;
    TextAlignment alignment_ = TextAlignment::Left;
};

} // namespace engine::ui
