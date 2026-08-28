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
    
    // Setup
    void setLabelText(const std::string& text);
    void setShowValueLabel(bool show);
    
    // Layout
    void layout() override;

private:
    UILabel* label_ = nullptr;
    UISlider* slider_ = nullptr;
    UILabel* valueLabel_ = nullptr;
};

} // namespace engine::ui
