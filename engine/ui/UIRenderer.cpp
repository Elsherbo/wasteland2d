#include "UIRenderer.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UIImage.h"
#include "UISlider.h"
#include "UICheckbox.h"
#include "UIScrollContainer.h"
#include <glm/vec2.hpp>

namespace engine::ui {

UIRenderer::UIRenderer(SDL_Renderer* renderer, render::TextRenderer& textRenderer, const render::Font& font, UIThemeManager& themeManager)
    : renderer_(renderer), textRenderer_(textRenderer), font_(font), themeManager_(themeManager) {
}

void UIRenderer::render(UIComponent* component) {
    if (!component || !component->isVisible()) {
        return;
    }
    
    // Call the component's renderUI method (polymorphic rendering)
    component->renderUI(*this);
}

render::Color UIRenderer::getStateColor(UIState state) const {
    switch (state) {
        case UIState::Hover:
            return hoverColor_;
        case UIState::Active:
            return activeColor_;
        case UIState::Disabled:
            return disabledColor_;
        case UIState::Focus:
            return activeColor_;
        default:
            return normalColor_;
    }
}

render::Color UIRenderer::getColorFromStyle(const UIStyle* style, UIState state, const render::Color& fallback) const {
    if (!style) {
        return fallback;
    }
    
    glm::vec4 color = style->getBackgroundColor(state, glm::vec4(
        fallback.r / 255.0f,
        fallback.g / 255.0f,
        fallback.b / 255.0f,
        fallback.a / 255.0f
    ));
    
    return render::Color{
        static_cast<uint8_t>(color.r * 255),
        static_cast<uint8_t>(color.g * 255),
        static_cast<uint8_t>(color.b * 255),
        static_cast<uint8_t>(color.a * 255)
    };
}

void UIRenderer::renderButtonHelper(UIButton* button) {
    glm::vec2 pos = button->getWorldPosition();
    glm::vec2 size = button->getWorldSize();
    
    // Get style from theme manager
    const UIStyle* style = themeManager_.getStyle("Button");
    
    // Determine color from style or fallback
    render::Color color = getColorFromStyle(style, button->getState(), getStateColor(button->getState()));
    if (button->isToggled()) {
        color = getColorFromStyle(style, UIState::Active, activeColor_);
    }
    
    // Draw button background
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &rect);
    
    // Draw border
    render::Color borderColor = render::Color{200, 200, 200, 255};
    if (style) {
        glm::vec4 borderVec = style->getBorderColor(button->getState(), glm::vec4(0.78f, 0.78f, 0.78f, 1.0f));
        borderColor = render::Color{
            static_cast<uint8_t>(borderVec.r * 255),
            static_cast<uint8_t>(borderVec.g * 255),
            static_cast<uint8_t>(borderVec.b * 255),
            static_cast<uint8_t>(borderVec.a * 255)
        };
    }
    SDL_SetRenderDrawColor(renderer_, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRectF(renderer_, &rect);
    
    // Draw text
    if (!button->getText().empty()) {
        render::Color textCol = textColor_;
        if (style) {
            glm::vec4 textVec = style->getTextColor(button->getState(), glm::vec4(1.0f));
            textCol = render::Color{
                static_cast<uint8_t>(textVec.r * 255),
                static_cast<uint8_t>(textVec.g * 255),
                static_cast<uint8_t>(textVec.b * 255),
                static_cast<uint8_t>(textVec.a * 255)
            };
        }
        
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, button->getText(), textCol, textW, textH);
        
        int textX = static_cast<int>(pos.x + (size.x - textW) / 2);
        int textY = static_cast<int>(pos.y + (size.y - textH) / 2);
        
        textRenderer_.draw(font_, button->getText(), textX, textY, textCol);
    }
}

void UIRenderer::renderLabelHelper(UILabel* label) {
    glm::vec2 pos = label->getWorldPosition();
    glm::vec2 size = label->getWorldSize();
    
    // Get style from theme manager
    const UIStyle* style = themeManager_.getStyle("Label");
    
    if (!label->getText().empty()) {
        render::Color textCol = textColor_;
        if (style) {
            glm::vec4 textVec = style->getTextColor(label->getState(), glm::vec4(1.0f));
            textCol = render::Color{
                static_cast<uint8_t>(textVec.r * 255),
                static_cast<uint8_t>(textVec.g * 255),
                static_cast<uint8_t>(textVec.b * 255),
                static_cast<uint8_t>(textVec.a * 255)
            };
        }
        
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, label->getText(), textCol, textW, textH);
        
        int textX = static_cast<int>(pos.x);
        int textY = static_cast<int>(pos.y);
        
        // Apply alignment
        if (label->getAlignment() == TextAlignment::Center) {
            textX = static_cast<int>(pos.x + (size.x - textW) / 2);
        } else if (label->getAlignment() == TextAlignment::Right) {
            textX = static_cast<int>(pos.x + size.x - textW);
        }
        
        textRenderer_.draw(font_, label->getText(), textX, textY, textCol);
    }
}

void UIRenderer::renderImageHelper(UIImage* image) {
    glm::vec2 pos = image->getWorldPosition();
    glm::vec2 size = image->getWorldSize();
    
    // For now, render as a colored rectangle with tint
    glm::vec4 tint = image->getTintColor();
    render::Color color{
        static_cast<uint8_t>(tint.r * 255),
        static_cast<uint8_t>(tint.g * 255),
        static_cast<uint8_t>(tint.b * 255),
        static_cast<uint8_t>(tint.a * 255)
    };
    
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
    SDL_RenderDrawRectF(renderer_, &rect);
}

void UIRenderer::renderSliderHelper(UISlider* slider) {
    glm::vec2 pos = slider->getWorldPosition();
    glm::vec2 size = slider->getWorldSize();
    
    // Get style from theme manager
    const UIStyle* style = themeManager_.getStyle("Slider");
    
    // Draw track
    SDL_FRect trackRect{pos.x, pos.y + size.y / 2 - 2, size.x, 4};
    SDL_SetRenderDrawColor(renderer_, 80, 80, 80, 255);
    SDL_RenderFillRectF(renderer_, &trackRect);
    
    // Calculate thumb position
    float value = slider->getValue();
    float thumbX = pos.x + value * (size.x - slider->getThumbSize());
    
    // Draw thumb
    float thumbSize = slider->getThumbSize();
    SDL_FRect thumbRect{thumbX, pos.y, thumbSize, size.y};
    
    render::Color thumbColor = getColorFromStyle(style, slider->getState(), getStateColor(slider->getState()));
    
    SDL_SetRenderDrawColor(renderer_, thumbColor.r, thumbColor.g, thumbColor.b, thumbColor.a);
    SDL_RenderFillRectF(renderer_, &thumbRect);
    
    // Draw thumb border
    SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
    SDL_RenderDrawRectF(renderer_, &thumbRect);
}

void UIRenderer::renderCheckboxHelper(UICheckbox* checkbox) {
    glm::vec2 pos = checkbox->getWorldPosition();
    glm::vec2 size = checkbox->getWorldSize();
    
    // Get style from theme manager
    const UIStyle* style = themeManager_.getStyle("Checkbox");
    
    // Draw checkbox box
    float boxSize = 20.0f;
    SDL_FRect boxRect{pos.x, pos.y, boxSize, boxSize};
    
    render::Color boxColor = normalColor_;
    if (checkbox->isChecked()) {
        boxColor = getColorFromStyle(style, UIState::Active, activeColor_);
    } else if (checkbox->getState() == UIState::Hover) {
        boxColor = getColorFromStyle(style, UIState::Hover, hoverColor_);
    }
    
    SDL_SetRenderDrawColor(renderer_, boxColor.r, boxColor.g, boxColor.b, boxColor.a);
    SDL_RenderFillRectF(renderer_, &boxRect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
    SDL_RenderDrawRectF(renderer_, &boxRect);
    
    // Draw checkmark if checked
    if (checkbox->isChecked()) {
        SDL_SetRenderDrawColor(renderer_, 50, 50, 50, 255);
        // Simple checkmark lines
        SDL_FRect check1{pos.x + 4, pos.y + 8, 4, 2};
        SDL_FRect check2{pos.x + 8, pos.y + 4, 8, 2};
        SDL_RenderFillRectF(renderer_, &check1);
        SDL_RenderFillRectF(renderer_, &check2);
    }
    
    // Draw text
    if (!checkbox->getText().empty()) {
        render::Color textCol = textColor_;
        if (style) {
            glm::vec4 textVec = style->getTextColor(checkbox->getState(), glm::vec4(1.0f));
            textCol = render::Color{
                static_cast<uint8_t>(textVec.r * 255),
                static_cast<uint8_t>(textVec.g * 255),
                static_cast<uint8_t>(textVec.b * 255),
                static_cast<uint8_t>(textVec.a * 255)
            };
        }
        
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, checkbox->getText(), textCol, textW, textH);
        
        int textX = static_cast<int>(pos.x + boxSize + 8);
        int textY = static_cast<int>(pos.y + (boxSize - textH) / 2);
        
        textRenderer_.draw(font_, checkbox->getText(), textX, textY, textCol);
    }
}

void UIRenderer::renderContainerHelper(UIContainer* container) {
    glm::vec2 pos = container->getWorldPosition();
    glm::vec2 size = container->getWorldSize();
    
    // Get style from theme manager
    const UIStyle* style = themeManager_.getStyle("Container");
    
    // Draw container background (subtle)
    render::Color bgColor = render::Color{30, 30, 35, 150};
    if (style) {
        glm::vec4 bgVec = style->getBackgroundColor(container->getState(), glm::vec4(0.12f, 0.12f, 0.14f, 0.59f));
        bgColor = render::Color{
            static_cast<uint8_t>(bgVec.r * 255),
            static_cast<uint8_t>(bgVec.g * 255),
            static_cast<uint8_t>(bgVec.b * 255),
            static_cast<uint8_t>(bgVec.a * 255)
        };
    }
    
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    SDL_SetRenderDrawColor(renderer_, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderFillRectF(renderer_, &rect);
    
    // Draw border
    render::Color borderColor = render::Color{100, 100, 100, 255};
    if (style) {
        glm::vec4 borderVec = style->getBorderColor(container->getState(), glm::vec4(0.39f, 0.39f, 0.39f, 1.0f));
        borderColor = render::Color{
            static_cast<uint8_t>(borderVec.r * 255),
            static_cast<uint8_t>(borderVec.g * 255),
            static_cast<uint8_t>(borderVec.b * 255),
            static_cast<uint8_t>(borderVec.a * 255)
        };
    }
    SDL_SetRenderDrawColor(renderer_, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRectF(renderer_, &rect);
    
    // Check if this is a ScrollContainer and set clip rect
    SDL_Rect oldClipRect;
    SDL_RenderGetClipRect(renderer_, &oldClipRect);
    bool hadClipRect = (oldClipRect.w != 0 && oldClipRect.h != 0);
    
    if (auto* scrollContainer = dynamic_cast<UIScrollContainer*>(container)) {
        glm::vec2 contentSize = scrollContainer->getContentSize();
        
        // Set clip rect to container bounds (will clip overflow content)
        SDL_Rect clipRect{
            static_cast<int>(pos.x),
            static_cast<int>(pos.y),
            static_cast<int>(size.x),
            static_cast<int>(size.y)
        };
        SDL_RenderSetClipRect(renderer_, &clipRect);
        
        // Render children
        for (auto* child : container->getChildren()) {
            child->renderUI(*this);
        }
        
        // Restore clip rect
        if (hadClipRect) {
            SDL_RenderSetClipRect(renderer_, &oldClipRect);
        } else {
            SDL_RenderSetClipRect(renderer_, nullptr);
        }
        
        // Render scrollbar
        renderScrollbarHelper(scrollContainer);
    } else {
        // Regular container - just render children
        for (auto* child : container->getChildren()) {
            child->renderUI(*this);
        }
    }
}

void UIRenderer::renderScrollbarHelper(UIScrollContainer* container) {
    glm::vec2 pos = container->getWorldPosition();
    glm::vec2 size = container->getWorldSize();
    glm::vec2 contentSize = container->getContentSize();
    
    float scrollbarWidth = 12.0f;
    float scrollbarHeight = size.y;
    float scrollbarX = pos.x + size.x - scrollbarWidth;
    
    if (contentSize.y > size.y) {
        glm::vec2 scrollPos = container->getScrollPosition();
        float maxScroll = contentSize.y - size.y;
        float scrollRatio = (maxScroll > 0) ? scrollPos.y / maxScroll : 0;
        float thumbRatio = size.y / contentSize.y;
        float thumbHeight = scrollbarHeight * thumbRatio;
        float thumbY = pos.y + (scrollbarHeight - thumbHeight) * scrollRatio;
        
        // Draw scrollbar track
        SDL_FRect trackRect{scrollbarX, pos.y, scrollbarWidth, scrollbarHeight};
        SDL_SetRenderDrawColor(renderer_, 50, 50, 55, 200);
        SDL_RenderFillRectF(renderer_, &trackRect);
        
        // Draw scrollbar thumb
        SDL_FRect thumbRect{scrollbarX, thumbY, scrollbarWidth, thumbHeight};
        SDL_SetRenderDrawColor(renderer_, 150, 150, 155, 255);
        SDL_RenderFillRectF(renderer_, &thumbRect);
    }
}

} // namespace engine::ui
