#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "UIComponent.h"
#include "UIStyle.h"
#include "UILayer.h"
#include "InputEvent.h"

namespace engine::ui {

// Forward declaration
class Renderer;

// UIManager - coordinates all UI elements
class UIManager {
public:
    UIManager();
    ~UIManager() = default;
    
    // Component creation
    template<typename T, typename... Args>
    T* createComponent(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        components_.push_back(std::move(component));
        return ptr;
    }
    
    // Theme management
    void setTheme(const std::string& themeName);
    UIStyle* getStyle(const std::string& componentName);
    const UIStyle* getStyle(const std::string& componentName) const;
    UIThemeManager& getThemeManager() { return themeManager_; }
    
    // Layer management
    UILayer* getLayer(const std::string& name);
    void addLayer(std::unique_ptr<UILayer> layer);
    
    // Hit testing
    UIComponent* hitTest(glm::vec2 position);
    
    // State management
    void updateStates(double dt);
    
    // Input routing (Godot-style)
    void dispatchInput(const InputEvent& event);
    bool handleInput(const InputEventLegacy& event);  // Legacy, for backward compatibility
    
    // Rendering
    void render(Renderer& renderer);
    
    // Update
    void update(double dt);
    
    // Focus management
    UIComponent* getFocusedComponent() const { return focusedComponent_; }
    void setFocusedComponent(UIComponent* component);
    void clearFocus();
    
    // Pointer capture management (for drag operations)
    UIComponent* getCapturedComponent() const { return capturedComponent_; }
    void setCapturedComponent(UIComponent* component);
    void clearCapture();
    
    // Hover management
    UIComponent* getHoveredComponent() const { return hoveredComponent_; }

private:
    std::vector<std::unique_ptr<UIComponent>> components_;
    UILayerManager layerManager_;
    UIThemeManager themeManager_;
    
    UIComponent* focusedComponent_ = nullptr;
    UIComponent* hoveredComponent_ = nullptr;
    UIComponent* capturedComponent_ = nullptr;  // For pointer capture during drag operations
};

} // namespace engine::ui
