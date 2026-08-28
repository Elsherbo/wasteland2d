#include "UIRenderer.h"
#include <glm/vec2.hpp>

namespace engine::ui {

UIRenderer::UIRenderer(SDL_Renderer* renderer, render::TextRenderer& textRenderer, const render::Font& font)
    : renderer_(renderer), textRenderer_(textRenderer), font_(font) {
}

void UIRenderer::render(UIComponent* component) {
    if (!component || !component->isVisible()) {
        return;
    }
    
    // Get component position and size
    glm::vec2 pos = component->getWorldPosition();
    glm::vec2 size = component->getWorldSize();
    
    // Determine color based on state
    render::Color color = normalColor_;
    UIState state = component->getState();
    
    switch (state) {
        case UIState::Hover:
            color = hoverColor_;
            break;
        case UIState::Active:
            color = activeColor_;
            break;
        case UIState::Disabled:
            color = disabledColor_;
            break;
        case UIState::Focus:
            color = activeColor_;
            break;
        default:
            color = normalColor_;
            break;
    }
    
    // Render based on component type
    if (auto* button = dynamic_cast<UIButton*>(component)) {
        renderButton(button);
    } else if (auto* label = dynamic_cast<UILabel*>(component)) {
        renderLabel(label);
    } else if (auto* image = dynamic_cast<UIImage*>(component)) {
        renderImage(image);
    } else if (auto* slider = dynamic_cast<UISlider*>(component)) {
        renderSlider(slider);
    } else if (auto* checkbox = dynamic_cast<UICheckbox*>(component)) {
        renderCheckbox(checkbox);
    } else if (auto* container = dynamic_cast<UIContainer*>(component)) {
        renderContainer(container);
    } else {
        // Default component rendering
        SDL_FRect rect{pos.x, pos.y, size.x, size.y};
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRectF(renderer_, &rect);
        
        // Draw border
        SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
        SDL_RenderDrawRectF(renderer_, &rect);
    }
    
    // Render children
    for (auto* child : component->getChildren()) {
        render(child);
    }
}

void UIRenderer::renderButton(UIButton* button) {
    glm::vec2 pos = button->getWorldPosition();
    glm::vec2 size = button->getWorldSize();
    
    // Determine color
    render::Color color = normalColor_;
    if (button->isToggled()) {
        color = activeColor_;
    } else {
        switch (button->getState()) {
            case UIState::Hover:
                color = hoverColor_;
                break;
            case UIState::Active:
                color = activeColor_;
                break;
            case UIState::Disabled:
                color = disabledColor_;
                break;
            default:
                color = normalColor_;
                break;
        }
    }
    
    // Draw button background
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
    SDL_RenderDrawRectF(renderer_, &rect);
    
    // Draw text
    if (!button->getText().empty()) {
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, button->getText(), textColor_, textW, textH);
        
        int textX = static_cast<int>(pos.x + (size.x - textW) / 2);
        int textY = static_cast<int>(pos.y + (size.y - textH) / 2);
        
        textRenderer_.draw(font_, button->getText(), textX, textY, textColor_);
    }
}

void UIRenderer::renderLabel(UILabel* label) {
    glm::vec2 pos = label->getWorldPosition();
    glm::vec2 size = label->getWorldSize();
    
    if (!label->getText().empty()) {
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, label->getText(), textColor_, textW, textH);
        
        int textX = static_cast<int>(pos.x);
        int textY = static_cast<int>(pos.y);
        
        // Apply alignment
        if (label->getAlignment() == TextAlignment::Center) {
            textX = static_cast<int>(pos.x + (size.x - textW) / 2);
        } else if (label->getAlignment() == TextAlignment::Right) {
            textX = static_cast<int>(pos.x + size.x - textW);
        }
        
        textRenderer_.draw(font_, label->getText(), textX, textY, textColor_);
    }
}

void UIRenderer::renderImage(UIImage* image) {
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

void UIRenderer::renderSlider(UISlider* slider) {
    glm::vec2 pos = slider->getWorldPosition();
    glm::vec2 size = slider->getWorldSize();
    
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
    
    render::Color thumbColor = normalColor_;
    if (slider->getState() == UIState::Active) {
        thumbColor = activeColor_;
    } else if (slider->getState() == UIState::Hover) {
        thumbColor = hoverColor_;
    }
    
    SDL_SetRenderDrawColor(renderer_, thumbColor.r, thumbColor.g, thumbColor.b, thumbColor.a);
    SDL_RenderFillRectF(renderer_, &thumbRect);
    
    // Draw thumb border
    SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
    SDL_RenderDrawRectF(renderer_, &thumbRect);
}

void UIRenderer::renderCheckbox(UICheckbox* checkbox) {
    glm::vec2 pos = checkbox->getWorldPosition();
    glm::vec2 size = checkbox->getWorldSize();
    
    // Draw checkbox box
    float boxSize = 20.0f;
    SDL_FRect boxRect{pos.x, pos.y, boxSize, boxSize};
    
    render::Color boxColor = normalColor_;
    if (checkbox->isChecked()) {
        boxColor = activeColor_;
    } else if (checkbox->getState() == UIState::Hover) {
        boxColor = hoverColor_;
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
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, checkbox->getText(), textColor_, textW, textH);
        
        int textX = static_cast<int>(pos.x + boxSize + 8);
        int textY = static_cast<int>(pos.y + (boxSize - textH) / 2);
        
        textRenderer_.draw(font_, checkbox->getText(), textX, textY, textColor_);
    }
}

void UIRenderer::renderContainer(UIContainer* container) {
    glm::vec2 pos = container->getWorldPosition();
    glm::vec2 size = container->getWorldSize();
    
    // Draw container background (subtle)
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    SDL_SetRenderDrawColor(renderer_, 30, 30, 35, 150);
    SDL_RenderFillRectF(renderer_, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderDrawRectF(renderer_, &rect);
    
    // Render children
    for (auto* child : container->getChildren()) {
        render(child);
    }
}

} // namespace engine::ui
