#pragma once

#include "UIComponent.h"
#include <string>
#include <functional>

namespace engine::ui {

// UICheckbox - toggle checkbox component
class UICheckbox : public UIComponent {
public:
    UICheckbox();
    ~UICheckbox() override = default;
    
    // State
    bool isChecked() const { return checked_; }
    void setChecked(bool checked);
    
    // Text
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; setDirty(); }
    
    // Callback
    std::function<void(bool)> onCheckedChanged;
    
    // Image support
    const std::string& getUncheckedImage() const { return uncheckedImage_; }
    void setUncheckedImage(const std::string& image) { uncheckedImage_ = image; setDirty(); }
    
    const std::string& getCheckedImage() const { return checkedImage_; }
    void setCheckedImage(const std::string& image) { checkedImage_ = image; setDirty(); }
    
    // Lifecycle
    void render(Renderer& renderer) override;
    void guiInput(const InputEvent& event) override;

private:
    bool checked_ = false;
    std::string text_;
    
    std::string uncheckedImage_;
    std::string checkedImage_;
};

} // namespace engine::ui
