#include "UIStyle.h"

namespace engine::ui {

// Easing function implementations
namespace Easing {
    float Linear(float t) { return t; }
    float EaseInQuad(float t) { return t * t; }
    float EaseOutQuad(float t) { return t * (2.0f - t); }
    float EaseInOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
    float EaseInCubic(float t) { return t * t * t; }
    float EaseOutCubic(float t) { return (--t) * t * t + 1.0f; }
    float EaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f; }
}

// UIStyle helper methods
glm::vec4 UIStyle::getBackgroundColor(UIState state, const glm::vec4& fallback) const {
    auto it = backgroundColor.find(state);
    if (it != backgroundColor.end()) {
        return it->second;
    }
    return fallback;
}

glm::vec4 UIStyle::getTextColor(UIState state, const glm::vec4& fallback) const {
    auto it = textColor.find(state);
    if (it != textColor.end()) {
        return it->second;
    }
    return fallback;
}

glm::vec4 UIStyle::getBorderColor(UIState state, const glm::vec4& fallback) const {
    auto it = borderColor.find(state);
    if (it != borderColor.end()) {
        return it->second;
    }
    return fallback;
}

std::string UIStyle::getBackgroundImage(UIState state) const {
    auto it = backgroundImage.find(state);
    if (it != backgroundImage.end()) {
        return it->second;
    }
    return "";
}

std::string UIStyle::getSprite(UIState state) const {
    auto it = sprite.find(state);
    if (it != sprite.end()) {
        return it->second;
    }
    return "";
}

// UIThemeManager implementation
void UIThemeManager::loadTheme(const std::string& name, const std::string& path) {
    // TODO: Implement JSON loading in Phase 6
    (void)name;
    (void)path;
}

void UIThemeManager::setTheme(const std::string& name) {
    currentTheme_ = name;
}

UIStyle* UIThemeManager::getStyle(const std::string& componentName) {
    auto it = styles_.find(componentName);
    if (it == styles_.end()) {
        // Create default style
        styles_[componentName] = UIStyle();
        return &styles_[componentName];
    }
    return &it->second;
}

const UIStyle* UIThemeManager::getStyle(const std::string& componentName) const {
    auto it = styles_.find(componentName);
    if (it == styles_.end()) {
        return nullptr;
    }
    return &it->second;
}

void UIThemeManager::registerStyle(const std::string& componentName, const UIStyle& style) {
    styles_[componentName] = style;
}

} // namespace engine::ui
