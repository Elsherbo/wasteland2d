#pragma once

#include <SDL.h>
#include <glm/vec2.hpp>
#include "UIComponent.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UIImage.h"
#include "UISlider.h"
#include "UICheckbox.h"
#include "UIContainer.h"
#include "UIVBox.h"
#include "UIHBox.h"
#include "UIGrid.h"
#include "UISliderContainer.h"
#include "UIScrollContainer.h"
#include "render/Color.h"
#include "render/Font.h"
#include "render/TextRenderer.h"

namespace engine::ui {

// Simple SDL renderer for UI components
class UIRenderer {
public:
    UIRenderer(SDL_Renderer* renderer, render::TextRenderer& textRenderer, const render::Font& font);
    ~UIRenderer() = default;
    
    // Render a component and all its children
    void render(UIComponent* component);
    
    // Set default colors for states
    void setNormalColor(const render::Color& color) { normalColor_ = color; }
    void setHoverColor(const render::Color& color) { hoverColor_ = color; }
    void setActiveColor(const render::Color& color) { activeColor_ = color; }
    void setDisabledColor(const render::Color& color) { disabledColor_ = color; }
    void setTextColor(const render::Color& color) { textColor_ = color; }

private:
    void renderButton(UIButton* button);
    void renderLabel(UILabel* label);
    void renderImage(UIImage* image);
    void renderSlider(UISlider* slider);
    void renderCheckbox(UICheckbox* checkbox);
    void renderContainer(UIContainer* container);
    
    SDL_Renderer* renderer_;
    render::TextRenderer& textRenderer_;
    const render::Font& font_;
    
    render::Color normalColor_ = {100, 100, 100, 255};
    render::Color hoverColor_ = {150, 150, 150, 255};
    render::Color activeColor_ = {200, 200, 200, 255};
    render::Color disabledColor_ = {50, 50, 50, 255};
    render::Color textColor_ = {255, 255, 255, 255};
};

} // namespace engine::ui
