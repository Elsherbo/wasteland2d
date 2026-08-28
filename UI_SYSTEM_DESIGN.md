# UI System Design for Wasteland2D

## Philosophy

Professional, extensible UI system inspired by:
- **Godot Control nodes** - VBox, HBox, Container system
- **Unity UI** - Canvas, RectTransform, Image/Button components
- **Dear ImGui** - Immediate mode patterns (for tools)
- **Web CSS** - Styling and layout concepts

## Core Design Principles

1. **Theming First** - All visual properties themeable (colors, images, fonts, animations)
2. **Image-Based** - Components support both colors and texture sprites
3. **Container System** - VBox, HBox, Grid, SliderContainer, etc.
4. **Extensible** - Easy to add new components without modifying core
5. **State-Aware** - Normal, Hover, Active, Disabled, Focus, Hidden
6. **Layered** - Z-ordering with explicit layers
7. **Hit Testing** - Mouse/keyboard input routing
8. **Dirty Rendering** - Only re-render when state changes

## Architecture

### UIComponent (Base Class)

```cpp
class UIComponent {
public:
    // States
    enum class State { Normal, Hover, Active, Disabled, Focus, Hidden };
    
    // Properties
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 anchor;  // 0,0 = top-left, 0.5,0.5 = center, 1,1 = bottom-right
    float rotation = 0.0f;
    glm::vec2 scale = glm::vec2(1.0f);
    State state = State::Normal;
    bool visible = true;
    bool interactable = true;
    
    // Styling
    std::shared_ptr<UIStyle> style;
    
    // Hierarchy
    UIComponent* parent = nullptr;
    std::vector<std::unique_ptr<UIComponent>> children;
    
    // Methods
    virtual void update(double dt);
    virtual void render(Renderer& renderer);
    virtual bool handleInput(const InputEvent& event);
    virtual void setState(State newState);
    
    // Layout
    virtual void layout();  // Calculate children positions
};
```

### UIStyle (Theming System)

```cpp
struct UIStyle {
    // Colors per state
    std::unordered_map<UIComponent::State, glm::vec4> backgroundColor;
    std::unordered_map<UIComponent::State, glm::vec4> textColor;
    std::unordered_map<UIComponent::State, glm::vec4> borderColor;
    
    // Images per state (can be null for color-only)
    std::unordered_map<UIComponent::State, std::string> backgroundImage;
    std::unordered_map<UIComponent::State, std::string> sprite;
    
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
    float transitionDuration = 0.15f;  // State transition time
    EasingFunction easing = Easing::EaseOutQuad;
};

// Theme management
class UIThemeManager {
public:
    void loadTheme(const std::string& name, const std::string& path);
    void setTheme(const std::string& name);
    UIStyle* getStyle(const std::string& componentName);
    
private:
    std::unordered_map<std::string, UIStyle> themes_;
    std::string currentTheme_;
};
```

### Container System (Like Godot)

```cpp
// Base container
class UIContainer : public UIComponent {
public:
    virtual void layout() override;
    
protected:
    glm::vec2 calculateContentArea();
};

// VBox - Vertical box layout
class UIVBox : public UIContainer {
public:
    float spacing = 0.0f;
    bool expandChildren = false;
    void layout() override;
};

// HBox - Horizontal box layout
class UIHBox : public UIContainer {
public:
    float spacing = 0.0f;
    bool expandChildren = false;
    void layout() override;
};

// Grid - 2D grid layout
class UIGrid : public UIContainer {
public:
    int columns = 1;
    int rows = 1;
    float cellWidth = 32.0f;
    float cellHeight = 32.0f;
    float spacing = 0.0f;
    void layout() override;
};

// SliderContainer - Slider with optional labels
class UISliderContainer : public UIContainer {
public:
    std::unique_ptr<UILabel> label;
    std::unique_ptr<UISlider> slider;
    std::unique_ptr<UILabel> valueLabel;
    void layout() override;
};

// ScrollContainer - Scrollable content
class UIScrollContainer : public UIContainer {
public:
    glm::vec2 contentSize;
    bool horizontalScroll = false;
    bool verticalScroll = true;
    void layout() override;
};
```

### Component Library

```cpp
// Basic components
class UIButton : public UIComponent {
public:
    std::string text;
    std::function<void()> onClick;
    bool toggleMode = false;  // Toggle button (on/off)
    
    // Image support
    std::string normalImage;
    std::string hoverImage;
    std::string activeImage;
};

class UILabel : public UIComponent {
public:
    std::string text;
    bool wordWrap = false;
    TextAlignment alignment = TextAlignment::Left;
};

class UIImage : public UIComponent {
public:
    std::string texturePath;
    bool preserveAspect = true;
    ImageMode mode = ImageMode::Fit;  // Fit, Fill, Stretch, Tile
};

class UISlider : public UIComponent {
public:
    float value = 0.5f;  // 0.0 to 1.0
    float minValue = 0.0f;
    float maxValue = 1.0f;
    std::function<void(float)> onValueChanged;
    
    // Styling
    std::string trackImage;
    std::string thumbImage;
    float thumbSize = 16.0f;
};

class UICheckbox : public UIComponent {
public:
    bool checked = false;
    std::function<void(bool)> onCheckedChanged;
    
    // Image support
    std::string uncheckedImage;
    std::string checkedImage;
};

class UITextField : public UIComponent {
public:
    std::string text;
    std::string placeholder;
    int maxLength = 0;  // 0 = unlimited
    PasswordMode passwordMode = PasswordMode::None;
    std::function<void(const std::string&)> onTextChanged;
};

class UIProgressBar : public UIComponent {
public:
    float value = 0.5f;  // 0.0 to 1.0
    std::string fillImage;  // Optional image for fill
    bool showValue = false;
};

class UITooltip : public UIComponent {
public:
    std::string text;
    float delay = 0.5f;  // Seconds before showing
    float followMouse = true;
};
```

### Layer System

```cpp
class UILayer {
public:
    std::string name;
    int zOrder;
    bool visible = true;
    std::vector<UIComponent*> components;
    
    void render(Renderer& renderer);
    void update(double dt);
    bool handleInput(const InputEvent& event);
};

class UILayerManager {
public:
    void addLayer(std::unique_ptr<UILayer> layer);
    void removeLayer(const std::string& name);
    UILayer* getLayer(const std::string& name);
    void renderAll(Renderer& renderer);
    void updateAll(double dt);
    
private:
    std::vector<std::unique_ptr<UILayer>> layers_;
};
```

### UIManager

```cpp
class UIManager {
public:
    // Component creation
    template<typename T, typename... Args>
    T* createComponent(Args&&... args);
    
    // Theme management
    void setTheme(const std::string& themeName);
    UIStyle* getStyle(const std::string& componentName);
    
    // Layer management
    UILayer* getLayer(const std::string& name);
    
    // Hit testing
    UIComponent* hitTest(glm::vec2 position);
    
    // State management
    void updateStates(double dt);
    
    // Input routing
    bool handleInput(const InputEvent& event);
    
    // Rendering
    void render(Renderer& renderer);
    
private:
    std::vector<std::unique_ptr<UIComponent>> components_;
    UILayerManager layerManager_;
    UIThemeManager themeManager_;
    UIComponent* focusedComponent_ = nullptr;
    UIComponent* hoveredComponent_ = nullptr;
};
```

### UISystem (Engine Integration)

```cpp
class UISystem : public SystemBase {
public:
    UISystem(UIManager& uiManager, InputManager& input);
    
    void update(double dt) override;
    void render() override;
    
private:
    UIManager& uiManager_;
    InputManager& input_;
};
```

## Theme File Format (JSON)

```json
{
  "name": "Default",
  "Button": {
    "backgroundColor": {
      "Normal": [0.2, 0.2, 0.2, 1.0],
      "Hover": [0.3, 0.3, 0.3, 1.0],
      "Active": [0.4, 0.4, 0.4, 1.0],
      "Disabled": [0.1, 0.1, 0.1, 0.5]
    },
    "backgroundImage": {
      "Normal": "button_normal.png",
      "Hover": "button_hover.png",
      "Active": "button_active.png"
    },
    "textColor": {
      "Normal": [1.0, 1.0, 1.0, 1.0],
      "Disabled": [0.5, 0.5, 0.5, 1.0]
    },
    "borderWidth": 1.0,
    "cornerRadius": 4.0,
    "padding": [8.0, 8.0],
    "fontSize": 16,
    "transitionDuration": 0.15
  },
  "Slider": {
    "trackImage": "slider_track.png",
    "thumbImage": "slider_thumb.png",
    "thumbSize": 16.0
  },
  "InventoryGrid": {
    "cellWidth": 32.0,
    "cellHeight": 32.0,
    "spacing": 2.0,
    "backgroundColor": [0.1, 0.1, 0.1, 0.8],
    "borderColor": [0.3, 0.3, 0.3, 1.0]
  }
}
```

## Implementation Order

### Phase 1: Core Infrastructure
1. UIComponent base class with states
2. UIStyle theming system
3. UILayerManager for z-ordering
4. UIManager basics

### Phase 2: Basic Components
5. UIButton (with image support)
6. UILabel
7. UIImage
8. UISlider
9. UICheckbox

### Phase 3: Container System
10. UIContainer base
11. UIVBox
12. UIHBox
13. UIGrid
14. UISliderContainer

### Phase 4: Advanced Components
15. UITextField
16. UIProgressBar
17. UITooltip
18. UIScrollContainer

### Phase 5: Game-Specific UI
19. Extract inventory UI using UIGrid
20. Extract HUD using containers
21. Extract interaction prompts
22. Integrate with GameScene

### Phase 6: Polish
23. Animation system
24. Theme loading from JSON
25. UI events for game communication
26. Performance optimization (dirty flags)

## Extensibility

Adding new components:
1. Inherit from UIComponent or UIContainer
2. Implement render(), handleInput(), layout()
3. Add style properties to UIStyle
4. Register with UIManager
5. Add to theme JSON

No modifications to core UI system needed!

## Benefits

- **Theming** - Complete visual customization without code changes
- **Image Support** - Professional sprite-based UI
- **Containers** - Easy layout management
- **Extensible** - Add components without touching core
- **Professional** - Matches Unity/Godot quality
- **Testable** - Each component testable independently
