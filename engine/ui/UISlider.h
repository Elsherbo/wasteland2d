#pragma once

#include "UIComponent.h"
#include <glm/vec2.hpp>
#include <string>
#include <functional>

namespace engine::ui {

// UISlider - range selection component
class UISlider : public UIComponent {
public:
    UISlider();
    ~UISlider() override = default;
    
    // Value
    float getValue() const { return value_; }
    void setValue(float value);
    
    float getMinValue() const { return minValue_; }
    void setMinValue(float min) { minValue_ = min; clampValue(); setDirty(); }
    
    float getMaxValue() const { return maxValue_; }
    void setMaxValue(float max) { maxValue_ = max; clampValue(); setDirty(); }
    
    // Callback
    std::function<void(float)> onValueChanged;
    
    // Styling
    const std::string& getTrackImage() const { return trackImage_; }
    void setTrackImage(const std::string& image) { trackImage_ = image; setDirty(); }
    
    const std::string& getThumbImage() const { return thumbImage_; }
    void setThumbImage(const std::string& image) { thumbImage_ = image; setDirty(); }
    
    float getThumbSize() const { return thumbSize_; }
    void setThumbSize(float size) { thumbSize_ = size; setDirty(); }
    
    // Lifecycle
    void render(Renderer& renderer) override;
    bool handleInput(const InputEvent& event) override;

private:
    void clampValue();
    glm::vec2 getThumbPosition() const;
    
    float value_ = 0.5f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    
    std::string trackImage_;
    std::string thumbImage_;
    float thumbSize_ = 16.0f;
    
    bool isDragging_ = false;
};

} // namespace engine::ui
