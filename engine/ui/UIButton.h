#pragma once

#include "UIComponent.h"
#include <string>
#include <functional>

namespace engine::ui {

// UIButton - clickable button with image support
class UIButton : public UIComponent {
public:
    UIButton();
    ~UIButton() override = default;
    
    // Text
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; setDirty(); }
    
    // Image support (per state)
    const std::string& getNormalImage() const { return normalImage_; }
    void setNormalImage(const std::string& image) { normalImage_ = image; setDirty(); }
    
    const std::string& getHoverImage() const { return hoverImage_; }
    void setHoverImage(const std::string& image) { hoverImage_ = image; setDirty(); }
    
    const std::string& getActiveImage() const { return activeImage_; }
    void setActiveImage(const std::string& image) { activeImage_ = image; setDirty(); }
    
    const std::string& getDisabledImage() const { return disabledImage_; }
    void setDisabledImage(const std::string& image) { disabledImage_ = image; setDirty(); }
    
    // Callback
    std::function<void()> onClick;
    
    // Toggle mode (on/off button)
    bool isToggleMode() const { return toggleMode_; }
    void setToggleMode(bool toggle) { toggleMode_ = toggle; }
    
    bool isToggled() const { return toggled_; }
    void setToggled(bool toggled);
    
    // Lifecycle
    void render(Renderer& renderer) override;
    void guiInput(const InputEvent& event) override;
    
protected:
    void onStateChanged(UIState oldState, UIState newState) override;

private:
    std::string text_;
    
    // Images per state
    std::string normalImage_;
    std::string hoverImage_;
    std::string activeImage_;
    std::string disabledImage_;
    
    // Toggle mode
    bool toggleMode_ = false;
    bool toggled_ = false;
    
    // Internal state
    bool wasPressed_ = false;
};

} // namespace engine::ui
