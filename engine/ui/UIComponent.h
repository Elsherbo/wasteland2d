#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "UIStyle.h"
#include "InputEvent.h"

namespace engine::ui {

// Forward declarations
class Renderer;
class UIRenderer;

// Mouse filter (Godot-style)
enum class MouseFilter {
    Stop,      // This control receives the event and consumes it, nothing below gets it
    Pass,      // This control receives the event and passes it to children
    Ignore     // This control is invisible to the mouse, pass straight through
};

// Size flags (Godot-style)
enum class SizeFlag {
    None = 0,
    Fill = 1 << 0,      // Fill available space
    Expand = 1 << 1,    // Expand to fill extra space
    ShrinkBegin = 1 << 2,
    ShrinkCenter = 1 << 3,
    ShrinkEnd = 1 << 4
};

inline SizeFlag operator|(SizeFlag a, SizeFlag b) {
    return static_cast<SizeFlag>(static_cast<int>(a) | static_cast<int>(b));
}

inline SizeFlag operator&(SizeFlag a, SizeFlag b) {
    return static_cast<SizeFlag>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool hasFlag(SizeFlag flags, SizeFlag flag) {
    return (static_cast<int>(flags) & static_cast<int>(flag)) != 0;
}

// Legacy input event (for backward compatibility with existing code)
struct InputEventLegacy {
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
    void setPosition(const glm::vec2& pos) { position_ = pos; setDirty(); invalidateParentLayout(); }
    
    const glm::vec2& getSize() const { return size_; }
    void setSize(const glm::vec2& size) { size_ = size; setDirty(); invalidateParentLayout(); }
    
    // Minimum size (for layout calculations)
    const glm::vec2& getMinSize() const { return minSize_; }
    void setMinSize(const glm::vec2& size) { minSize_ = size; setDirty(); invalidateParentLayout(); }
    
    // Custom minimum size (overrides calculated min size)
    const glm::vec2& getCustomMinimumSize() const { return customMinimumSize_; }
    void setCustomMinimumSize(const glm::vec2& size) { customMinimumSize_ = size; setDirty(); invalidateParentLayout(); }
    
    // Calculate minimum size based on content (override in subclasses)
    virtual glm::vec2 calculateMinSize() const;
    
    // World transform (includes parent transform)
    glm::vec2 getWorldPosition() const;
    glm::vec2 getWorldSize() const;
    
    const glm::vec2& getAnchor() const { return anchor_; }
    void setAnchor(const glm::vec2& anchor) { anchor_ = anchor; setDirty(); }
    
    // Four-point anchors (Godot-style)
    const glm::vec2& getAnchorLeftTop() const { return anchorLeftTop_; }
    void setAnchorLeftTop(const glm::vec2& anchor) { anchorLeftTop_ = anchor; setDirty(); invalidateParentLayout(); }
    
    const glm::vec2& getAnchorRightBottom() const { return anchorRightBottom_; }
    void setAnchorRightBottom(const glm::vec2& anchor) { anchorRightBottom_ = anchor; setDirty(); invalidateParentLayout(); }
    
    // Margins/offsets
    const glm::vec4& getMargins() const { return margins_; }  // (left, top, right, bottom)
    void setMargins(const glm::vec4& margins) { margins_ = margins; setDirty(); invalidateParentLayout(); }
    
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
    virtual void renderUI(UIRenderer& uiRenderer);  // New: UIRenderer-specific rendering
    virtual bool handleInput(const InputEventLegacy& event);  // Legacy, for backward compatibility
    virtual void layout();  // Calculate children positions
    
    // GUI Input (Godot-style _gui_input)
    virtual void guiInput(const InputEvent& event);
    
    // Pointer capture request (for drag operations)
    void requestPointerCapture();
    void releasePointerCapture();
    
    // Event propagation control
    void acceptEvent() { eventAccepted_ = true; }
    bool isEventAccepted() const { return eventAccepted_; }
    void resetEventAccepted() { eventAccepted_ = false; }
    
    // Mouse filter (Godot-style)
    MouseFilter getMouseFilter() const { return mouseFilter_; }
    void setMouseFilter(MouseFilter filter) { mouseFilter_ = filter; }
    
    // Dirty flag for optimization
    bool isDirty() const { return dirty_; }
    void setDirty() { dirty_ = true; }
    void clearDirty() { dirty_ = false; }
    
    // Layout dirty flag (for containers)
    virtual bool isLayoutDirty() const { return false; }
    virtual void setLayoutDirty() {}
    virtual void clearLayoutDirty() {}
    
    // Invalidate parent layout (call when this component's size/structure changes)
    void invalidateParentLayout();
    
    // Hit testing
    virtual bool containsPoint(glm::vec2 point) const;
    
    // Z-order
    int getZOrder() const { return zOrder_; }
    void setZOrder(int zOrder) { zOrder_ = zOrder; }
    
    // Size flags (for layout)
    SizeFlag getSizeFlags() const { return sizeFlags_; }
    void setSizeFlags(SizeFlag flags) { sizeFlags_ = flags; setDirty(); invalidateParentLayout(); }

protected:
    // State change callback
    virtual void onStateChanged(UIState oldState, UIState newState);
    
    // Transform
    glm::vec2 position_ = glm::vec2(0.0f);
    glm::vec2 size_ = glm::vec2(100.0f, 100.0f);
    glm::vec2 minSize_ = glm::vec2(0.0f);
    glm::vec2 customMinimumSize_ = glm::vec2(0.0f);
    glm::vec2 anchor_ = glm::vec2(0.0f);  // Legacy pivot (top-left)
    glm::vec2 anchorLeftTop_ = glm::vec2(0.0f);  // New: left/top anchor ratios
    glm::vec2 anchorRightBottom_ = glm::vec2(1.0f, 1.0f);  // New: right/bottom anchor ratios
    glm::vec4 margins_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);  // (left, top, right, bottom)
    float rotation_ = 0.0f;
    glm::vec2 scale_ = glm::vec2(1.0f);
    
    // State
    UIState state_ = UIState::Normal;
    bool interactable_ = true;
    SizeFlag sizeFlags_ = SizeFlag::None;
    
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
    
    // Event handling
    bool eventAccepted_ = false;
    MouseFilter mouseFilter_ = MouseFilter::Pass;
};

} // namespace engine::ui
