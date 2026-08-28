#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <unordered_map>
#include <functional>

namespace engine::ui {

// UI States
enum class UIState {
    Normal,    // Default state
    Hover,     // Mouse over element
    Active,    // Currently being used/pressed
    Disabled,  // Not interactable
    Focus,     // Keyboard focus
    Hidden     // Not visible
};

// Text alignment
enum class TextAlignment {
    Left,
    Center,
    Right
};

// Easing functions for animations
namespace Easing {
    using EasingFunction = float(*)(float);
    
    float Linear(float t);
    float EaseInQuad(float t);
    float EaseOutQuad(float t);
    float EaseInOutQuad(float t);
    float EaseInCubic(float t);
    float EaseOutCubic(float t);
    float EaseInOutCubic(float t);
}

// UI Style for theming
class UIStyle {
public:
    UIStyle() = default;
    
    // Colors per state
    std::unordered_map<UIState, glm::vec4> backgroundColor;
    std::unordered_map<UIState, glm::vec4> textColor;
    std::unordered_map<UIState, glm::vec4> borderColor;
    
    // Images per state (empty string = use color)
    std::unordered_map<UIState, std::string> backgroundImage;
    std::unordered_map<UIState, std::string> sprite;
    
    // Dimensions
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    glm::vec2 padding = glm::vec2(0.0f);
    glm::vec2 margin = glm::vec2(0.0f);
    
    // Typography
    std::string font = "default";
    int fontSize = 16;
    glm::vec4 fontColor = glm::vec4(1.0f);
    TextAlignment alignment = TextAlignment::Left;
    
    // Effects
    bool dropShadow = false;
    glm::vec4 shadowColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
    glm::vec2 shadowOffset = glm::vec2(2.0f, 2.0f);
    
    // Animation
    float transitionDuration = 0.15f;  // State transition time in seconds
    Easing::EasingFunction easing = Easing::EaseOutQuad;
    
    // Helper to get color for a state (with fallback)
    glm::vec4 getBackgroundColor(UIState state, const glm::vec4& fallback = glm::vec4(0.5f)) const;
    glm::vec4 getTextColor(UIState state, const glm::vec4& fallback = glm::vec4(1.0f)) const;
    glm::vec4 getBorderColor(UIState state, const glm::vec4& fallback = glm::vec4(0.0f)) const;
    std::string getBackgroundImage(UIState state) const;
    std::string getSprite(UIState state) const;
};

// Theme manager for loading and switching themes
class UIThemeManager {
public:
    UIThemeManager() = default;
    
    // Load theme from JSON file (TODO: Phase 6)
    void loadTheme(const std::string& name, const std::string& path);
    
    // Set current theme
    void setTheme(const std::string& name);
    
    // Get style for a component (creates default if not found)
    UIStyle* getStyle(const std::string& componentName);
    const UIStyle* getStyle(const std::string& componentName) const;
    
    // Register a style programmatically
    void registerStyle(const std::string& componentName, const UIStyle& style);
    
    // Get current theme name
    const std::string& getCurrentTheme() const { return currentTheme_; }

private:
    std::unordered_map<std::string, UIStyle> styles_;
    std::string currentTheme_ = "Default";
};

} // namespace engine::ui
