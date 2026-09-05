# UI API Quick Reference

Quick lookup for common UI component APIs.

## Components

### UIButton
```cpp
setText("Text")
getText()
setToggleMode(bool)
isToggleMode()
setToggled(bool)
isToggled()
setNormalImage("path")
setHoverImage("path")
setActiveImage("path")
setStyleName("Button.Primary")
onClick = []() {}
```

### UILabel
```cpp
setText("Text")
getText()
setWordWrap(bool)
isWordWrap()
setAlignment(TextAlignment::Center)
getAlignment()
setHeading(bool)
isHeading()
```

### UIImage
```cpp
setTexturePath("path")
getTexturePath()
setTintColor(glm::vec4(r,g,b,a))
getTintColor()
setMode(ImageMode::Fit)
getMode()
setPreserveAspect(bool)
preserveAspect()
```

### UISlider
```cpp
setValue(0.5f)
getValue()
setMinValue(0.0f)
getMinValue()
setMaxValue(1.0f)
getMaxValue()
setThumbSize(20.0f)
getThumbSize()
setTrackImage("path")
setThumbImage("path")
onValueChanged = [](float v) {}
```

### UICheckbox
```cpp
setText("Label")
getText()
setChecked(true)
isChecked()
setCheckedImage("path")
setUncheckedImage("path")
onCheckedChanged = [](bool c) {}
```

### UISliderContainer
```cpp
setLabelText("Volume")
getLabelText()
setShowValueLabel(true)
getShowValueLabel()
getSlider()
getLabel()
getValueLabel()
onValueChanged = [](float v) {}
```

### UIContainer
```cpp
setSpacing(10.0f)
getSpacing()
setPadding(glm::vec2(10, 10))
getPadding()
setAutoSize(true)
getAutoSize()
setStyleName("Panel")
getStyleName()
addChild(std::move(child))
getChildren()
```

### UIVBox
```cpp
// Inherits UIContainer
// Stacks children vertically
```

### UIHBox
```cpp
// Inherits UIContainer
// Stacks children horizontally
```

### UIGrid
```cpp
setColumns(5)
getColumns()
setRows(4)
getRows()
setCellWidth(64.0f)
getCellWidth()
setCellHeight(64.0f)
getCellHeight()
setSpacing(4.0f)
getSpacing()
```

### UIScrollContainer
```cpp
setContentSize(glm::vec2(400, 600))
getContentSize()
setScrollPosition(glm::vec2(0, 100))
getScrollPosition()
setHorizontalScrollEnabled(true)
isHorizontalScrollEnabled()
setVerticalScrollEnabled(true)
isVerticalScrollEnabled()
```

## Size Flags

```cpp
enum class SizeFlag {
    None = 0,
    Fill = 1 << 0,   // Fill cross-axis space
    Expand = 1 << 1  // Expand to share extra space
};

// Usage
child->setSizeFlags(SizeFlag::Fill);
child->setSizeFlags(SizeFlag::Fill | SizeFlag::Expand);
```

## UI States

```cpp
enum class UIState {
    Normal,   // Default state
    Hover,    // Mouse over
    Active,   // Mouse pressed
    Disabled, // Not interactable
    Focus,    // Keyboard focus
    Hidden    // Not visible
};
```

## Theme Manager

```cpp
// Create style
UIStyle style;
style.backgroundColor[UIState::Normal] = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
style.textColor[UIState::Normal] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
style.cornerRadius = 4.0f;

// Register
themeManager.registerStyle("Default", "Button", style);

// Use default theme
installDefaultDarkTheme(themeManager, uiRenderer);
```

## UIManager

```cpp
// Initialize
UIManager uiManager;
auto& themeManager = uiManager.getThemeManager();

// Get layer
UILayer* layer = uiManager.getLayer("UI");

// Register component
layer->addComponent(root.get());

// Update (call every frame)
uiManager.update(dt);

// Dispatch input
uiManager.dispatchInput(inputEvent);
```

## UIRenderer

```cpp
// Initialize
UIRenderer renderer(sdlRenderer, textRenderer, font, themeManager, &headingFont);

// Render
renderer.render(rootComponent);

// Configure
renderer.setCornerRadius(6.0f);
renderer.setAccentColor({76, 141, 255, 255});
```

## Input Events

```cpp
// Mouse motion
InputEvent::mouseMotion(x, y, relX, relY)

// Mouse button
InputEvent::mouseButton(x, y, button, pressed)

// Mouse wheel
InputEvent::mouseWheel(x, y, delta)

// Keyboard
InputEvent::keyboard(key, pressed)
```

## Common Patterns

### Create Button
```cpp
auto button = std::make_unique<UIButton>();
button->setText("Click");
button->setSize(glm::vec2(120, 30));
button->setStyleName("Button.Primary");
button->onClick = []() { /* action */ };
```

### Create Section
```cpp
auto section = std::make_unique<UIVBox>();
section->setStyleName("Section");
section->setSpacing(10);
section->setPadding(glm::vec2(15));
section->setSizeFlags(SizeFlag::Fill);
```

### Create Scrollable Content
```cpp
auto scroll = std::make_unique<UIScrollContainer>();
scroll->setSize(glm::vec2(400, 300));
scroll->setVerticalScrollEnabled(true);

auto content = std::make_unique<UIVBox>();
content->setAutoSize(true);
// ... add children ...

scroll->addChild(std::move(content));
scroll->layout();
```

### Always Layout After Changes
```cpp
container->addChild(std::move(child));
container->layout();  // Critical!
```

### Register with Layer
```cpp
uiManager.getLayer("UI")->addComponent(root.get());
```

## Log Categories

```cpp
LogCategory::UI          // UI components
LogCategory::Core        // Core systems
LogCategory::Render      // Rendering
LogCategory::Input       // Input handling
LogCategory::Audio       // Audio system
```

## Default Theme Styles

- `Button` - Standard button
- `Button.Primary` - Emphasized action
- `Button.Ghost` - Secondary action
- `Label` - Standard text
- `Label.Heading` - Section titles
- `Slider` - Slider control
- `Checkbox` - Checkbox control
- `Panel` - Main container
- `Section` - Subsection
- `Transparent` - No background
- `Container` - Generic container
