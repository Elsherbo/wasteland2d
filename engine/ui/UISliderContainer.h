#pragma once

#include "UIContainer.h"
#include "UISlider.h"
#include "UILabel.h"
#include <memory>

namespace engine::ui {

// UISliderContainer - slider with optional labels
class UISliderContainer : public UIContainer {
public:
    UISliderContainer();
    ~UISliderContainer() override = default;
    
    // Access to components
    UISlider* getSlider() const { return slider_; }
    UILabel* getLabel() const { return label_; }
    UILabel* getValueLabel() const { return valueLabel_; }
    
    // External callback (called after internal update)
    std::function<void(float)> onValueChanged;
    
    // Setup
    void setLabelText(const std::string& text);
    void setShowValueLabel(bool show);
    void setSliderValue(float value);
    void updateValueLabel();
    
    // Layout
    void layout() override;

private:
    UILabel* label_ = nullptr;
    UISlider* slider_ = nullptr;
    UILabel* valueLabel_ = nullptr;
    
    std::string lastValueText_;  // Cache to prevent unnecessary updates
};

} // namespace engine::ui
