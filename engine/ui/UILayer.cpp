#include "UILayer.h"
#include <algorithm>

namespace engine::ui {

UILayer::UILayer(const std::string& name, int zOrder)
    : name_(name), zOrder_(zOrder) {
}

void UILayer::addComponent(UIComponent* component) {
    components_.push_back(component);
}

void UILayer::removeComponent(UIComponent* component) {
    auto it = std::find(components_.begin(), components_.end(), component);
    if (it != components_.end()) {
        components_.erase(it);
    }
}

void UILayer::update(double dt) {
    if (!visible_) return;
    
    for (auto* component : components_) {
        if (component->isVisible()) {
            component->update(dt);
        }
    }
}

void UILayer::render(Renderer& renderer) {
    if (!visible_) return;
    
    for (auto* component : components_) {
        if (component->isVisible()) {
            component->render(renderer);
        }
    }
}

bool UILayer::handleInput(const InputEvent& event) {
    if (!visible_) return false;
    
    // Pass input to components (reverse order for top-most first)
    for (auto it = components_.rbegin(); it != components_.rend(); ++it) {
        if ((*it)->isVisible() && (*it)->isInteractable()) {
            if ((*it)->handleInput(event)) {
                return true;  // Input handled
            }
        }
    }
    return false;  // Input not handled
}

// UILayerManager implementation
void UILayerManager::addLayer(std::unique_ptr<UILayer> layer) {
    layers_.push_back(layer.get());
    layersOwned_.push_back(std::move(layer));
    sortLayers();
}

void UILayerManager::removeLayer(const std::string& name) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [&name](UILayer* layer) { return layer->getName() == name; });
    
    if (it != layers_.end()) {
        // Remove from owned
        auto ownedIt = std::find_if(layersOwned_.begin(), layersOwned_.end(),
            [it](const std::unique_ptr<UILayer>& ptr) { return ptr.get() == *it; });
        if (ownedIt != layersOwned_.end()) {
            layersOwned_.erase(ownedIt);
        }
        
        layers_.erase(it);
    }
}

UILayer* UILayerManager::getLayer(const std::string& name) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [&name](UILayer* layer) { return layer->getName() == name; });
    return (it != layers_.end()) ? *it : nullptr;
}

const UILayer* UILayerManager::getLayer(const std::string& name) const {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [&name](UILayer* layer) { return layer->getName() == name; });
    return (it != layers_.end()) ? *it : nullptr;
}

void UILayerManager::renderAll(Renderer& renderer) {
    for (auto* layer : layers_) {
        if (layer->isVisible()) {
            layer->render(renderer);
        }
    }
}

void UILayerManager::updateAll(double dt) {
    for (auto* layer : layers_) {
        if (layer->isVisible()) {
            layer->update(dt);
        }
    }
}

bool UILayerManager::handleInput(const InputEvent& event) {
    // Pass input to layers in reverse order (top-most first)
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if ((*it)->isVisible()) {
            if ((*it)->handleInput(event)) {
                return true;  // Input handled
            }
        }
    }
    return false;  // Input not handled
}

void UILayerManager::sortLayers() {
    std::sort(layers_.begin(), layers_.end(),
        [](UILayer* a, UILayer* b) { return a->getZOrder() < b->getZOrder(); });
}

} // namespace engine::ui
