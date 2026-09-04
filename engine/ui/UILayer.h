#pragma once

#include <string>
#include <vector>
#include <memory>
#include "UIComponent.h"

namespace engine::ui {

// Forward declaration
class Renderer;

// UI Layer for z-ordering
class UILayer {
public:
    UILayer(const std::string& name, int zOrder);
    ~UILayer() = default;
    
    const std::string& getName() const { return name_; }
    int getZOrder() const { return zOrder_; }
    void setZOrder(int zOrder) { zOrder_ = zOrder; }
    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }
    
    // Component management
    void addComponent(UIComponent* component);
    void removeComponent(UIComponent* component);
    const std::vector<UIComponent*>& getComponents() const { return components_; }
    
    // Lifecycle
    void update(double dt);
    void layout();
    void render(Renderer& renderer);
    bool handleInput(const InputEventLegacy& event);

private:
    std::string name_;
    int zOrder_;
    bool visible_ = true;
    std::vector<UIComponent*> components_;
};

// Layer manager for coordinating multiple layers
class UILayerManager {
public:
    UILayerManager() = default;
    ~UILayerManager() = default;
    
    // Layer management
    void addLayer(std::unique_ptr<UILayer> layer);
    void removeLayer(const std::string& name);
    UILayer* getLayer(const std::string& name);
    const UILayer* getLayer(const std::string& name) const;
    
    // Rendering (renders in z-order)
    void renderAll(Renderer& renderer);
    
    // Updates all visible layers
    void updateAll(double dt);
    
    // Re-applies layout for any component that's been marked dirty since
    // the last pass (scrolling, dynamic content, resizing, ...). Must run
    // every frame, not just once at startup -- see UILayer::layout().
    void layoutAll();
    
    // Input handling (top-most visible layer first)
    bool handleInput(const InputEventLegacy& event);
    
    // Get all layers
    const std::vector<UILayer*>& getLayers() const { return layers_; }

private:
    std::vector<std::unique_ptr<UILayer>> layersOwned_;
    std::vector<UILayer*> layers_;  // Sorted by z-order
    
    void sortLayers();  // Sort layers by z-order
};

} // namespace engine::ui
