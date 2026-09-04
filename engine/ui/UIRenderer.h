#pragma once

#include <SDL.h>
#include <glm/vec2.hpp>
#include "UIComponent.h"
#include "UIStyle.h"
#include "render/Color.h"
#include "render/Font.h"
#include "render/TextRenderer.h"

namespace engine::ui {

// Forward declarations
class UIButton;
class UILabel;
class UIImage;
class UISlider;
class UICheckbox;
class UIContainer;
class UIScrollContainer;

// Simple SDL renderer for UI components
class UIRenderer {
public:
    UIRenderer(SDL_Renderer* renderer, render::TextRenderer& textRenderer, const render::Font& font,
               UIThemeManager& themeManager, const render::Font* headingFont = nullptr);
    ~UIRenderer() = default;
    
    // Render a component and all its children
    void render(UIComponent* component);
    
    // Helper methods for components to use in their renderUI implementations
    void renderButtonHelper(UIButton* button);
    void renderLabelHelper(UILabel* label);
    void renderImageHelper(UIImage* image);
    void renderSliderHelper(UISlider* slider);
    void renderCheckboxHelper(UICheckbox* checkbox);
    void renderContainerHelper(UIContainer* container);
    void renderScrollbarHelper(UIScrollContainer* container);
    
    // Color helpers
    render::Color getStateColor(UIState state) const;
    render::Color getColorFromStyle(const UIStyle* style, UIState state, const render::Color& fallback) const;
    
    // Theme manager access
    UIThemeManager& getThemeManager() { return themeManager_; }
    const UIThemeManager& getThemeManager() const { return themeManager_; }
    
    // Set default colors for states (fallback if theme doesn't specify)
    void setNormalColor(const render::Color& color) { normalColor_ = color; }
    void setHoverColor(const render::Color& color) { hoverColor_ = color; }
    void setActiveColor(const render::Color& color) { activeColor_ = color; }
    void setDisabledColor(const render::Color& color) { disabledColor_ = color; }
    void setTextColor(const render::Color& color) { textColor_ = color; }
    void setAccentColor(const render::Color& color) { accentColor_ = color; }
    
    // Corner radius (px) applied to panels/cards/buttons/etc when drawing
    // via the rounded-rect helpers below. 0 = sharp corners.
    void setCornerRadius(float radius) { cornerRadius_ = radius; }

private:
    // Rounded-rectangle primitives. SDL2 has no built-in rounded rect;
    // the fill is built from 3 axis-aligned rects (the "plus" shape minus
    // corners) plus 4 triangle-fan quarter-circles via SDL_RenderGeometry
    // (SDL >= 2.0.18) for the corners themselves; the border is 4 straight
    // edges plus 4 polyline arcs, which needs no version gate. Both fall
    // back to a plain sharp rect if radius is ~0 or SDL_RenderGeometry
    // isn't available.
    void fillRoundedRect(const SDL_FRect& rect, float radius, render::Color color);
    void drawRoundedRectBorder(const SDL_FRect& rect, float radius, render::Color color);
    
    // Thick line segment (extruded quad) -- used for the checkbox checkmark,
    // which needs actual diagonal strokes rather than 1px SDL lines.
    void fillThickLine(glm::vec2 from, glm::vec2 to, float thickness, render::Color color);
    
    SDL_Renderer* renderer_;
    render::TextRenderer& textRenderer_;
    const render::Font& font_;
    const render::Font* headingFont_;  // falls back to font_ if null
    UIThemeManager& themeManager_;
    
    render::Color normalColor_ = {100, 100, 100, 255};
    render::Color hoverColor_ = {150, 150, 150, 255};
    render::Color activeColor_ = {200, 200, 200, 255};
    render::Color disabledColor_ = {50, 50, 50, 255};
    render::Color textColor_ = {255, 255, 255, 255};
    render::Color accentColor_ = {76, 141, 255, 255};
    float cornerRadius_ = 6.0f;
};

} // namespace engine::ui