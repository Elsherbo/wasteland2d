#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "UIStyle.h"

namespace engine::ui {

// Forward declarations
class Renderer;

// Dummy InputEvent for now (will be expanded later)
struct InputEvent {
    glm::vec2 mousePosition;
    bool mouseDown = false;
    bool mouseUp = false;
};

// Forward declaration
class UIStyle;

// Base class for all UI components
class UIComponent {
public:
    UIComponent();
    virtual ~UIComponent() = default;
    
    // State management
    UIState getState() const { return state_; }
    void setState(UIState newState);
    bool isVisible() const { return state_ != UIState::Hidden; }
    void setVisible(bool visible);
    bool isInteractable() const { return interactable_; }
    void setInteractable(bool interactable) { interactable_ = interactable; }
    
    // Properties
    const glm::vec2& getPosition() const { return position_; }
    void setPosition(const glm::vec2& pos) { position_ = pos; setDirty(); }
    
    const glm::vec2& getSize() const { return size_; }
    void setSize(const glm::vec2& size) { size_ = size; setDirty(); }
    
    // World transform (includes parent transform)
    glm::vec2 getWorldPosition() const;
    glm::vec2 getWorldSize() const;
    
    const glm::vec2& getAnchor() const { return anchor_; }
    void setAnchor(const glm::vec2& anchor) { anchor_ = anchor; setDirty(); }
    
    float getRotation() const { return rotation_; }
    void setRotation(float rotation) { rotation_ = rotation; setDirty(); }
    
    const glm::vec2& getScale() const { return scale_; }
    void setScale(const glm::vec2& scale) { scale_ = scale; setDirty(); }
    
    // Styling (TODO: Phase 2)
    // void setStyle(std::shared_ptr<UIStyle> style) { style_ = style; setDirty(); }
    // UIStyle* getStyle() const { return style_.get(); }
    
    // Hierarchy
    UIComponent* getParent() const { return parent_; }
    void addChild(std::unique_ptr<UIComponent> child);
    void removeChild(UIComponent* child);
    const std::vector<UIComponent*>& getChildren() const { return children_; }
    
    // Lifecycle
    virtual void update(double dt);
    virtual void render(Renderer& renderer);
    virtual bool handleInput(const InputEvent& event);
    virtual void layout();  // Calculate children positions
    
    // Dirty flag for optimization
    bool isDirty() const { return dirty_; }
    void setDirty() { dirty_ = true; }
    void clearDirty() { dirty_ = false; }
    
    // Hit testing
    virtual bool containsPoint(glm::vec2 point) const;
    
    // Z-order
    int getZOrder() const { return zOrder_; }
    void setZOrder(int zOrder) { zOrder_ = zOrder; }

protected:
    // State change callback
    virtual void onStateChanged(UIState oldState, UIState newState);

private:
    // Transform
    glm::vec2 position_ = glm::vec2(0.0f);
    glm::vec2 size_ = glm::vec2(100.0f, 100.0f);
    glm::vec2 anchor_ = glm::vec2(0.0f);  // Top-left
    float rotation_ = 0.0f;
    glm::vec2 scale_ = glm::vec2(1.0f);
    
    // State
    UIState state_ = UIState::Normal;
    bool interactable_ = true;
    
    // Styling (TODO: Phase 2)
    // std::shared_ptr<UIStyle> style_;
    
    // Hierarchy
    UIComponent* parent_ = nullptr;
    std::vector<UIComponent*> children_;  // Raw pointers for iteration
    std::vector<std::unique_ptr<UIComponent>> childrenOwned_;
    
    // Z-order
    int zOrder_ = 0;
    
    // Dirty flag
    bool dirty_ = true;
};

} // namespace engine::ui
