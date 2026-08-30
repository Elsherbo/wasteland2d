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
    return getStyle(currentTheme_, componentName);
}

const UIStyle* UIThemeManager::getStyle(const std::string& componentName) const {
    return getStyle(currentTheme_, componentName);
}

UIStyle* UIThemeManager::getStyle(const std::string& themeName, const std::string& componentName) {
    auto themeIt = themeStyles_.find(themeName);
    if (themeIt == themeStyles_.end()) {
        // Create theme if it doesn't exist
        themeStyles_[themeName] = std::unordered_map<std::string, UIStyle>();
        themeIt = themeStyles_.find(themeName);
    }
    
    auto& styles = themeIt->second;
    auto it = styles.find(componentName);
    if (it == styles.end()) {
        // Create default style
        styles[componentName] = UIStyle();
        return &styles[componentName];
    }
    return &it->second;
}

const UIStyle* UIThemeManager::getStyle(const std::string& themeName, const std::string& componentName) const {
    auto themeIt = themeStyles_.find(themeName);
    if (themeIt == themeStyles_.end()) {
        return nullptr;
    }
    
    const auto& styles = themeIt->second;
    auto it = styles.find(componentName);
    if (it == styles.end()) {
        return nullptr;
    }
    return &it->second;
}

void UIThemeManager::registerStyle(const std::string& themeName, const std::string& componentName, const UIStyle& style) {
    themeStyles_[themeName][componentName] = style;
}

bool UIThemeManager::hasTheme(const std::string& name) const {
    return themeStyles_.find(name) != themeStyles_.end();
}

} // namespace engine::ui
