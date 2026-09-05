# Wasteland2D UI System Guide

A professional retained-mode UI framework inspired by Godot's Control system, with a focus on clean architecture, theming, and ease of use.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Concepts](#core-concepts)
3. [Component Reference](#component-reference)
4. [Theming System](#theming-system)
5. [Layout System](#layout-system)
6. [Input Handling](#input-handling)
7. [Usage Examples](#usage-examples)
8. [Best Practices](#best-practices)
9. [Common Patterns](#common-patterns)

---

## Architecture Overview

The UI system is built around a hierarchical component tree with retained-mode rendering:

```
UIManager
├── UILayerManager
│   ├── UILayer (UI)
│   ├── UILayer (HUD)
│   └── UILayer (Dialogs)
└── UIThemeManager
    └── UIStyle (per component/state)
```

**Key Design Principles:**
- **Retained-mode**: Components persist as objects with state, unlike immediate-mode redraw-every-frame
- **Hierarchical**: Parent-child relationships define layout and input routing
- **Theme-driven**: Visual appearance separated from logic via the theme system
- **Godot-inspired**: Control nodes, containers, and layout flags mirror Godot's patterns
- **Polymorphic rendering**: Each component implements `renderUI()` instead of a central switch

---

## Core Concepts

### UIComponent

The base class for all UI elements. Every UI control inherits from this.

**Key Properties:**
- `position`: Screen position (glm::vec2)
- `size`: Component dimensions (glm::vec2)
- `visible`: Whether the component is rendered
- `interactable`: Whether the component receives input
- `state`: Current UI state (Normal, Hover, Active, Disabled, Focus, Hidden)
- `zOrder`: Rendering order (higher = drawn on top)

**Lifecycle:**
```cpp
// Create
auto button = std::make_unique<UIButton>();
button->setPosition(glm::vec2(100, 100));
button->setSize(glm::vec2(120, 30));

// Add to parent
container->addChild(std::move(button));

// Layout (must be called after structure changes)
container->layout();
```

### Size Flags

Control how children are sized within containers:

```cpp
enum class SizeFlag {
    None = 0,      // Use natural size
    Fill = 1 << 0, // Fill available space on cross axis
    Expand = 1 << 1 // Expand to share extra space on main axis
};
```

**VBox:**
- `Fill`: Child width fills container width
- `Expand`: Child height shares extra vertical space

**HBox:**
- `Fill`: Child height fills container height
- `Expand`: Child width shares extra horizontal space

### Auto-Size

Containers can automatically resize to fit their content:

```cpp
auto container = std::make_unique<UIVBox>();
container->setAutoSize(true);  // Resizes to fit children
container->layout();           // Size calculated automatically
```

---

## Component Reference

### UIButton

A clickable button with support for toggle mode and per-state images.

**API:**
```cpp
// Text
void setText(const std::string& text);
const std::string& getText() const;

// Toggle mode
void setToggleMode(bool enabled);
bool isToggleMode() const;
void setToggled(bool toggled);
bool isToggled() const;

// Images per state
void setNormalImage(const std::string& path);
void setHoverImage(const std::string& path);
void setActiveImage(const std::string& path);

// Style variant
void setStyleName(const std::string& name);  // "Button", "Button.Primary", "Button.Ghost"

// Callbacks
std::function<void()> onClick;
```

**Example:**
```cpp
auto button = std::make_unique<UIButton>();
button->setText("Click Me");
button->setSize(glm::vec2(120, 30));
button->setStyleName("Button.Primary");
button->onClick = []() {
    LOG_INFO(LogCategory::UI, "Button clicked!");
};
```

### UILabel

Text display with alignment and word wrap support.

**API:**
```cpp
void setText(const std::string& text);
const std::string& getText() const;

void setWordWrap(bool enabled);
bool isWordWrap() const;

void setAlignment(TextAlignment alignment);  // Left, Center, Right
TextAlignment getAlignment() const;

void setHeading(bool enabled);  // Uses larger font and "Label.Heading" style
bool isHeading() const;
```

**Example:**
```cpp
auto title = std::make_unique<UILabel>();
title->setText("Settings");
title->setHeading(true);
title->setAlignment(TextAlignment::Center);
```

### UIImage

Image display with tinting and scaling modes.

**API:**
```cpp
void setTexturePath(const std::string& path);
const std::string& getTexturePath() const;

void setTintColor(glm::vec4 color);
glm::vec4 getTintColor() const;

void setMode(ImageMode mode);  // Stretch, Fill, Fit, Center
ImageMode getMode() const;

void setPreserveAspect(bool preserve);
bool preserveAspect() const;
```

**Example:**
```cpp
auto icon = std::make_unique<UIImage>();
icon->setTexturePath("assets/icons/settings.png");
icon->setTintColor(glm::vec4(1, 1, 1, 1));
icon->setMode(ImageMode::Fit);
```

### UISlider

Horizontal slider for value selection.

**API:**
```cpp
void setValue(float value);
float getValue() const;

void setMinValue(float min);
float getMinValue() const;

void setMaxValue(float max);
float getMaxValue() const;

void setThumbSize(float size);
float getThumbSize() const;

void setTrackImage(const std::string& path);
void setThumbImage(const std::string& path);

std::function<void(float)> onValueChanged;
```

**Example:**
```cpp
auto slider = std::make_unique<UISlider>();
slider->setMinValue(0.0f);
slider->setMaxValue(100.0f);
slider->setValue(50.0f);
slider->onValueChanged = [](float value) {
    LOG_INFO(LogCategory::UI, "Slider: {}", value);
};
```

### UICheckbox

Toggleable checkbox with label.

**API:**
```cpp
void setText(const std::string& text);
const std::string& getText() const;

void setChecked(bool checked);
bool isChecked() const;

void setCheckedImage(const std::string& path);
void setUncheckedImage(const std::string& path);

std::function<void(bool)> onCheckedChanged;
```

**Example:**
```cpp
auto checkbox = std::make_unique<UICheckbox>();
checkbox->setText("Enable Feature");
checkbox->setChecked(true);
checkbox->onCheckedChanged = [](bool checked) {
    LOG_INFO(LogCategory::UI, "Checkbox: {}", checked);
};
```

### UISliderContainer

Composite widget combining label, slider, and value label.

**API:**
```cpp
void setLabelText(const std::string& text);
const std::string& getLabelText() const;

void setShowValueLabel(bool show);
bool getShowValueLabel() const;

// Access to internal components
UISlider* getSlider();
UILabel* getLabel();
UILabel* getValueLabel();

// Callback
std::function<void(float)> onValueChanged;
```

**Example:**
```cpp
auto volumeSlider = std::make_unique<UISliderContainer>();
volumeSlider->setLabelText("Master Volume");
volumeSlider->setShowValueLabel(true);
volumeSlider->getSlider()->setValue(0.75f);
volumeSlider->onValueChanged = [](float value) {
    audioSystem->setMasterVolume(value);
};
```

### UIContainer

Base container for organizing child components.

**API:**
```cpp
// Spacing and padding
void setSpacing(float spacing);
float getSpacing() const;

void setPadding(glm::vec2 padding);
glm::vec2 getPadding() const;

// Auto-size
void setAutoSize(bool enabled);
bool getAutoSize() const;

// Style role
void setStyleName(const std::string& name);  // "Panel", "Section", "Header"
const std::string& getStyleName() const;

// Children
void addChild(std::unique_ptr<UIComponent> child);
const std::vector<UIComponent*>& getChildren() const;
```

### UIVBox

Vertical box container - stacks children top to bottom.

**API:**
```cpp
// Inherits from UIContainer
// + layout() and calculateMinSize() overrides
```

**Example:**
```cpp
auto vbox = std::make_unique<UIVBox>();
vbox->setSpacing(10);
vbox->setPadding(glm::vec2(15));

auto label = std::make_unique<UILabel>();
label->setText("Title");
label->setSizeFlags(SizeFlag::Fill);  // Fill width
vbox->addChild(std::move(label));

vbox->layout();
```

### UIHBox

Horizontal box container - stacks children left to right.

**API:**
```cpp
// Inherits from UIContainer
// + layout() and calculateMinSize() overrides
```

**Example:**
```cpp
auto hbox = std::make_unique<UIHBox>();
hbox->setSpacing(8);

auto button1 = std::make_unique<UIButton>();
button1->setText("OK");
button1->setSizeFlags(SizeFlag::Fill);  // Fill height

auto button2 = std::make_unique<UIButton>();
button2->setText("Cancel");
button2->setSizeFlags(SizeFlag::Fill);

hbox->addChild(std::move(button1));
hbox->addChild(std::move(button2));
```

### UIGrid

Grid container for 2D layouts.

**API:**
```cpp
void setColumns(int columns);
int getColumns() const;

void setRows(int rows);
int getRows() const;

void setCellWidth(float width);
float getCellWidth() const;

void setCellHeight(float height);
float getCellHeight() const;

void setSpacing(float spacing);
float getSpacing() const;
```

**Example:**
```cpp
auto grid = std::make_unique<UIGrid>();
grid->setColumns(4);
grid->setRows(3);
grid->setCellWidth(64.0f);
grid->setCellHeight(64.0f);
grid->setSpacing(4.0f);

for (int i = 0; i < 12; ++i) {
    auto item = std::make_unique<UIImage>();
    item->setTexturePath("item.png");
    grid->addChild(std::move(item));
}
```

### UIScrollContainer

Scrollable viewport for content larger than available space.

**API:**
```cpp
void setContentSize(glm::vec2 size);
glm::vec2 getContentSize() const;

void setScrollPosition(glm::vec2 pos);
glm::vec2 getScrollPosition() const;

void setHorizontalScrollEnabled(bool enabled);
bool isHorizontalScrollEnabled() const;

void setVerticalScrollEnabled(bool enabled);
bool isVerticalScrollEnabled() const;
```

**Example:**
```cpp
auto scroll = std::make_unique<UIScrollContainer>();
scroll->setSize(glm::vec2(400, 300));
scroll->setContentSize(glm::vec2(400, 600));  // Content is taller
scroll->setVerticalScrollEnabled(true);

auto content = std::make_unique<UIVBox>();
content->setAutoSize(true);  // Auto-derive content size
// ... add children ...

scroll->addChild(std::move(content));
```

---

## Theming System

The theme system separates visual appearance from logic using `UIStyle` and `UIThemeManager`.

### UIStyle

Defines appearance for a component type across states:

```cpp
struct UIStyle {
    glm::vec4 backgroundColor[6];  // Per-state colors
    glm::vec4 textColor[6];
    glm::vec4 borderColor[6];
    float borderWidth = 1.0f;
    float cornerRadius = 0.0f;
};
```

### Creating a Theme

```cpp
auto& themeManager = uiManager.getThemeManager();

// Button style
UIStyle buttonStyle;
buttonStyle.backgroundColor[UIState::Normal] = glm::vec4(0.39f, 0.39f, 0.39f, 1.0f);
buttonStyle.backgroundColor[UIState::Hover] = glm::vec4(0.59f, 0.59f, 0.59f, 1.0f);
buttonStyle.textColor[UIState::Normal] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
buttonStyle.cornerRadius = 4.0f;

themeManager.registerStyle("Default", "Button", buttonStyle);

// Primary button variant
UIStyle primaryStyle = buttonStyle;
primaryStyle.backgroundColor[UIState::Normal] = glm::vec4(0.3f, 0.5f, 0.9f, 1.0f);
themeManager.registerStyle("Default", "Button.Primary", primaryStyle);
```

### Using Style Names

```cpp
auto button = std::make_unique<UIButton>();
button->setStyleName("Button.Primary");  // Uses "Button.Primary" style
```

### Default Theme

The built-in `installDefaultDarkTheme()` provides a professional dark theme:

```cpp
#include "ui/UIDefaultTheme.h"

engine::ui::installDefaultDarkTheme(themeManager, uiRenderer);
```

**Included styles:**
- `Button`, `Button.Primary`, `Button.Ghost`
- `Label`, `Label.Heading`
- `Slider`, `Checkbox`
- `Panel`, `Section`, `Transparent`, `Container`

---

## Layout System

### Layout Pass

Layout is a two-pass process:

1. **Measure Pass**: Calculate minimum sizes from bottom to top
2. **Arrange Pass**: Position children with calculated sizes

```cpp
// Always call layout() after structural changes
container->addChild(std::move(child));
container->layout();  // Recalculates layout
```

### Layout Invalidation

Changes that affect layout automatically invalidate the hierarchy:

```cpp
// These trigger layout recalculation on next layout() call
component->setSize(newSize);
component->setPosition(newPos);
container->setPadding(newPadding);
container->setSpacing(newSpacing);
```

### Per-Frame Layout

UIManager calls `layoutAll()` every frame to handle dynamic content:

```cpp
// Called automatically in UIManager::update()
layerManager_.layoutAll();
```

---

## Input Handling

### Input Event Types

```cpp
InputEvent::mouseMotion(x, y, relX, relY)
InputEvent::mouseButton(x, y, button, pressed)
InputEvent::mouseWheel(x, y, delta)
InputEvent::keyboard(key, pressed)
```

### Input Routing

Input flows from top to bottom through the component tree:

1. Event dispatched to root component
2. Component checks if it contains the point
3. If yes, passes to children (reverse order, top-most first)
4. Child that accepts the event stops propagation
5. Parent marks itself as accepted

```cpp
// In your game loop
if (event.type == SDL_MOUSEMOTION) {
    InputEvent inputEvent = InputEvent::mouseMotion(
        event.motion.x, event.motion.y,
        event.motion.xrel, event.motion.yrel
    );
    uiManager.dispatchInput(inputEvent);
}
```

### Hit Testing

Components automatically check `containsPoint()` before processing input:

```cpp
bool UIComponent::containsPoint(glm::vec2 point) const {
    glm::vec2 pos = getWorldPosition();
    glm::vec2 size = getWorldSize();
    return point.x >= pos.x && point.x < pos.x + size.x &&
           point.y >= pos.y && point.y < pos.y + size.y;
}
```

---

## Usage Examples

### Example 1: Simple Settings Panel

```cpp
#include "ui/UIVBox.h"
#include "ui/UISliderContainer.h"
#include "ui/UICheckbox.h"
#include "ui/UIButton.h"
#include "ui/UIDefaultTheme.h"

// Initialize
engine::ui::UIManager uiManager;
auto& themeManager = uiManager.getThemeManager();
engine::ui::UIRenderer uiRenderer(renderer, textRenderer, font, themeManager);
engine::ui::installDefaultDarkTheme(themeManager, uiRenderer);

// Create panel
auto panel = std::make_unique<engine::ui::UIVBox>();
panel->setPosition(glm::vec2(50, 50));
panel->setSize(glm::vec2(400, 300));
panel->setSpacing(15);
panel->setPadding(glm::vec2(20));
panel->setStyleName("Panel");

// Volume slider
auto volume = std::make_unique<engine::ui::UISliderContainer>();
volume->setLabelText("Volume");
volume->setSizeFlags(engine::ui::SizeFlag::Fill);
volume->onValueChanged = [](float value) {
    audioSystem->setVolume(value);
};
panel->addChild(std::move(volume));

// VSync checkbox
auto vsync = std::make_unique<engine::ui::UICheckbox>();
vsync->setText("VSync");
vsync->setChecked(true);
vsync->onCheckedChanged = [](bool checked) {
    window->setVSync(checked);
};
panel->addChild(std::move(vsync));

// Apply button
auto apply = std::make_unique<engine::ui::UIButton>();
apply->setText("Apply");
apply->setStyleName("Button.Primary");
apply->onClick = []() {
    settings->save();
};
panel->addChild(std::move(apply));

// Layout and register
panel->layout();
uiManager.getLayer("UI")->addComponent(panel.get());
```

### Example 2: Scrollable List

```cpp
auto scroll = std::make_unique<engine::ui::UIScrollContainer>();
scroll->setSize(glm::vec2(300, 200));
scroll->setVerticalScrollEnabled(true);

auto content = std::make_unique<engine::ui::UIVBox>();
content->setSpacing(5);
content->setAutoSize(true);

for (int i = 0; i < 20; ++i) {
    auto item = std::make_unique<engine::ui::UILabel>();
    item->setText("Item " + std::to_string(i));
    content->addChild(std::move(item));
}

scroll->addChild(std::move(content));
scroll->layout();
```

### Example 3: Grid Inventory

```cpp
auto grid = std::make_unique<engine::ui::UIGrid>();
grid->setColumns(5);
grid->setRows(4);
grid->setCellWidth(64.0f);
grid->setCellHeight(64.0f);
grid->setSpacing(4.0f);

for (auto& item : inventory->getItems()) {
    auto slot = std::make_unique<engine::ui::UIImage>();
    slot->setTexturePath(item->getIconPath());
    slot->setMode(engine::ui::ImageMode::Fit);
    grid->addChild(std::move(slot));
}

grid->layout();
```

### Example 4: Dialog with Actions

```cpp
auto dialog = std::make_unique<engine::ui::UIVBox>();
dialog->setStyleName("Panel");
dialog->setSpacing(10);
dialog->setPadding(glm::vec2(20));

// Message
auto message = std::make_unique<engine::ui::UILabel>();
message->setText("Are you sure?");
message->setAlignment(engine::ui::TextAlignment::Center);
dialog->addChild(std::move(message));

// Actions row
auto actions = std::make_unique<engine::ui::UIHBox>();
actions->setSpacing(10);

auto yes = std::make_unique<engine::ui::UIButton>();
yes->setText("Yes");
yes->setStyleName("Button.Primary");
yes->onClick = []() { confirm(); };

auto no = std::make_unique<engine::ui::UIButton>();
no->setText("No");
no->setStyleName("Button.Ghost");
no->onClick = []() { cancel(); };

actions->addChild(std::move(yes));
actions->addChild(std::move(no));
dialog->addChild(std::move(actions));

dialog->layout();
```

---

## Best Practices

### 1. Always Call layout() After Structural Changes

```cpp
container->addChild(std::move(child));
container->layout();  // Don't forget this!
```

### 2. Use Size Flags for Responsive Layouts

```cpp
// Good: Child fills parent width
child->setSizeFlags(SizeFlag::Fill);

// Avoid: Hard-coding sizes that break on resize
child->setSize(glm::vec2(400, 30));  // Brittle
```

### 3. Leverage Auto-Size for Content-Driven Layouts

```cpp
// Good: Container sizes to fit content
container->setAutoSize(true);

// Avoid: Manually calculating sizes
container->setSize(calculateManualSize());  // Error-prone
```

### 4. Use Style Names for Visual Hierarchy

```cpp
primaryButton->setStyleName("Button.Primary");
secondaryButton->setStyleName("Button.Ghost");
sectionPanel->setStyleName("Section");
```

### 5. Register Components with UILayer

```cpp
// Required for input and layout updates
uiManager.getLayer("UI")->addComponent(root.get());
```

### 6. Use Proper Log Categories

```cpp
LOG_INFO(LogCategory::UI, "Button clicked");
LOG_WARNING(LogCategory::UI, "Invalid slider value");
```

### 7. Initialize Theme Before Creating Components

```cpp
// Good: Theme first
installDefaultDarkTheme(themeManager, uiRenderer);
auto button = std::make_unique<UIButton>();

// Avoid: Components won't have styles
auto button = std::make_unique<UIButton>();
installDefaultDarkTheme(themeManager, uiRenderer);
```

---

## Common Patterns

### Pattern 1: Settings Panel

```cpp
auto createSettingsPanel = [&]() {
    auto panel = std::make_unique<UIVBox>();
    panel->setStyleName("Panel");
    panel->setSpacing(12);
    panel->setPadding(glm::vec2(16));

    // Helper for sections
    auto addSection = [&](const std::string& title) {
        auto section = std::make_unique<UIVBox>();
        section->setStyleName("Section");
        section->setSpacing(8);
        section->setPadding(glm::vec2(12));
        section->setSizeFlags(SizeFlag::Fill);

        auto label = std::make_unique<UILabel>();
        label->setText(title);
        label->setHeading(true);
        label->setSizeFlags(SizeFlag::Fill);
        section->addChild(std::move(label));

        return section;
    };

    // Graphics section
    auto graphics = addSection("Graphics");
    auto vsync = std::make_unique<UICheckbox>();
    vsync->setText("VSync");
    graphics->addChild(std::move(vsync));
    panel->addChild(std::move(graphics));

    panel->layout();
    return panel;
};
```

### Pattern 2: Reusable Button Factory

```cpp
auto createButton = [](const std::string& text, 
                        const std::string& style = "Button",
                        std::function<void()> onClick = nullptr) {
    auto button = std::make_unique<UIButton>();
    button->setText(text);
    button->setStyleName(style);
    button->onClick = std::move(onClick);
    return button;
};

auto primary = createButton("Save", "Button.Primary", []() { save(); });
auto cancel = createButton("Cancel", "Button.Ghost", []() { close(); });
```

### Pattern 3: Dynamic List Building

```cpp
auto buildList = [](const std::vector<Item>& items) {
    auto list = std::make_unique<UIVBox>();
    list->setSpacing(4);
    list->setAutoSize(true);

    for (const auto& item : items) {
        auto row = std::make_unique<UIHBox>();
        row->setSpacing(8);

        auto icon = std::make_unique<UIImage>();
        icon->setTexturePath(item.icon);
        icon->setSize(glm::vec2(32, 32));

        auto label = std::make_unique<UILabel>();
        label->setText(item.name);
        label->setSizeFlags(SizeFlag::Fill);

        row->addChild(std::move(icon));
        row->addChild(std::move(label));
        list->addChild(std::move(row));
    }

    return list;
};
```

### Pattern 4: Modal Dialog

```cpp
auto showConfirmDialog = [&](const std::string& message,
                              std::function<void()> onConfirm) {
    auto dialog = std::make_unique<UIVBox>();
    dialog->setStyleName("Panel");
    dialog->setAutoSize(true);
    dialog->setSpacing(12);
    dialog->setPadding(glm::vec2(20));

    auto msg = std::make_unique<UILabel>();
    msg->setText(message);
    msg->setAlignment(TextAlignment::Center);
    dialog->addChild(std::move(msg));

    auto actions = std::make_unique<UIHBox>();
    actions->setSpacing(8);

    auto yes = createButton("Yes", "Button.Primary", onConfirm);
    auto no = createButton("No", "Button.Ghost", [this]() {
        closeDialog();
    });

    actions->addChild(std::move(yes));
    actions->addChild(std::move(no));
    dialog->addChild(std::move(actions));

    dialog->layout();
    showModal(std::move(dialog));
};
```

---

## Integration with Game Loop

```cpp
// In your game update loop
void Game::update(double dt) {
    // Update UI
    uiManager.update(dt);
    
    // Render UI
    uiRenderer.render(rootComponent);
    
    // Handle input
    if (event.type == SDL_MOUSEMOTION) {
        InputEvent evt = InputEvent::mouseMotion(...);
        uiManager.dispatchInput(evt);
    }
    // ... other input types
}
```

---

## Testing UI Components

Unit tests demonstrate component usage:

```cpp
// tests/ui_container_test.cpp
auto vbox = std::make_unique<UIVBox>();
vbox->setSpacing(5.0f);
assert(vbox->getSpacing() == 5.0f);

auto child = std::make_unique<UIButton>();
child->setSizeFlags(SizeFlag::Fill);
vbox->addChild(std::move(child));
vbox->layout();
```

---

## Future Enhancements

Planned features for the UI system:

- [ ] Focus navigation (keyboard/controller)
- [ ] Animations and transitions
- [ ] Drag and drop support
- [ ] Rich text rendering
- [ ] Canvas layers with z-order
- [ ] Resource-based theme loading (JSON)
- [ ] Accessibility features
- [ ] Layout constraints system
- [ ] Data binding
- [ ] UI designer tool

---

## References

- **Inspired by**: Godot Engine's Control system
- **Math Library**: GLM (OpenGL Mathematics)
- **Rendering**: SDL2
- **Text**: SDL_ttf

---

*Last updated: 2026-09-05*
