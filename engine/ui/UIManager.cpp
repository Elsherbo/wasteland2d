#include "UIManager.h"

namespace engine::ui {

UIManager::UIManager() {
    // Create default layers
    auto backgroundLayer = std::make_unique<UILayer>("Background", 0);
    auto gameLayer = std::make_unique<UILayer>("Game", 10);
    auto uiLayer = std::make_unique<UILayer>("UI", 20);
    auto overlayLayer = std::make_unique<UILayer>("Overlay", 30);
    auto modalLayer = std::make_unique<UILayer>("Modal", 40);
    
    layerManager_.addLayer(std::move(backgroundLayer));
    layerManager_.addLayer(std::move(gameLayer));
    layerManager_.addLayer(std::move(uiLayer));
    layerManager_.addLayer(std::move(overlayLayer));
    layerManager_.addLayer(std::move(modalLayer));
}

void UIManager::setTheme(const std::string& themeName) {
    themeManager_.setTheme(themeName);
}

UIStyle* UIManager::getStyle(const std::string& componentName) {
    return themeManager_.getStyle(componentName);
}

const UIStyle* UIManager::getStyle(const std::string& componentName) const {
    return themeManager_.getStyle(componentName);
}

UILayer* UIManager::getLayer(const std::string& name) {
    return layerManager_.getLayer(name);
}

void UIManager::addLayer(std::unique_ptr<UILayer> layer) {
    layerManager_.addLayer(std::move(layer));
}

UIComponent* UIManager::hitTest(glm::vec2 position) {
    // Check layers in reverse order (top-most first)
    const auto& layers = layerManager_.getLayers();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        if ((*it)->isVisible()) {
            const auto& components = (*it)->getComponents();
            for (auto compIt = components.rbegin(); compIt != components.rend(); ++compIt) {
                if ((*compIt)->isVisible() && (*compIt)->isInteractable()) {
                    if ((*compIt)->containsPoint(position)) {
                        return *compIt;
                    }
                }
            }
        }
    }
    return nullptr;
}

void UIManager::updateStates(double dt) {
    // Update hover state based on mouse position
    // TODO: Implement proper mouse position tracking
    (void)dt;
}

void UIManager::dispatchInput(const InputEvent& event) {
    // Godot-style event dispatching
    // 1. Reset event accepted flags
    std::function<void(UIComponent*)> resetFlags = [&](UIComponent* comp) {
        comp->resetEventAccepted();
        for (auto* child : comp->getChildren()) {
            resetFlags(child);
        }
    };
    
    const auto& layers = layerManager_.getLayers();
    for (auto* layer : layers) {
        if (layer->isVisible()) {
            for (auto* comp : layer->getComponents()) {
                resetFlags(comp);
            }
        }
    }
    
    // 2. Dispatch to components in z-order (top-most first)
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        if (!(*it)->isVisible()) continue;
        
        const auto& components = (*it)->getComponents();
        for (auto compIt = components.rbegin(); compIt != components.rend(); ++compIt) {
            UIComponent* comp = *compIt;
            if (!comp->isVisible() || !comp->isInteractable()) continue;
            
            // Check mouse filter
            if (event.isMouseMotion() || event.isMouseButton()) {
                if (comp->getMouseFilter() == MouseFilter::Stop) continue;
            }
            
            // Check if event is within component bounds
            if (event.isMouseMotion() || event.isMouseButton()) {
                const auto* mouseData = event.getMouseData();
                if (mouseData && !comp->containsPoint(mouseData->position)) continue;
            }
            
            // Call guiInput
            comp->guiInput(event);
            
            // If event was accepted, stop propagation
            if (comp->isEventAccepted()) {
                return;
            }
            
            // If mouse filter is Ignore, stop propagation even if not accepted
            if (event.isMouseMotion() || event.isMouseButton()) {
                if (comp->getMouseFilter() == MouseFilter::Ignore) return;
            }
        }
    }
}

bool UIManager::handleInput(const InputEventLegacy& event) {
    return layerManager_.handleInput(event);
}

void UIManager::render(Renderer& renderer) {
    layerManager_.renderAll(renderer);
}

void UIManager::update(double dt) {
    layerManager_.updateAll(dt);
    updateStates(dt);
}

void UIManager::setFocusedComponent(UIComponent* component) {
    // Clear focus from previous component
    if (focusedComponent_) {
        focusedComponent_->setState(UIState::Normal);
    }
    
    focusedComponent_ = component;
    
    // Set focus on new component
    if (focusedComponent_) {
        focusedComponent_->setState(UIState::Focus);
    }
}

void UIManager::clearFocus() {
    if (focusedComponent_) {
        focusedComponent_->setState(UIState::Normal);
        focusedComponent_ = nullptr;
    }
}

} // namespace engine::ui
