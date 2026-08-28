# Game UI System Documentation

## Overview

The wasteland2d game already has a well-structured, professional UI system in `game/ui/` that is functional and working. The UI system is separate from the new `engine/ui/` UI framework (Phases 1-3) which provides general-purpose UI components.

## Existing Game UI System

### Files
- `game/ui/DragDropController.h` - Drag and drop logic for inventory
- `game/ui/InventoryRenderer.h` - Inventory rendering with grid layout
- `game/ui/EquipmentRenderer.h` - Equipment slots and hotbar rendering
- `engine/ui/GridLayout.h` - Grid cell mathematics (engine-level)

### Features

**1. Drag and Drop System**
- `DragDropController` class manages item dragging
- `HeldStack` struct tracks dragged items
- `DropOutcome` enum: Invalid, Place, Merge, Swap
- Grab offset tracking for smooth cursor following
- Rotation support during drag
- Merge and swap logic

**2. Inventory Rendering**
- `renderInventoryContents()` - Renders all stacks as colored rectangles
- `renderStackQuantities()` - Shows quantity > 1 in bottom-right corner
- `renderWeightReadout()` - Shows "used / max kg" weight display
- `renderItemNameTooltip()` - Shows item name on hover
- `renderHeldStack()` - Renders dragged item following cursor

**3. Equipment & Hotbar**
- `renderEquipmentSlots()` - 4 equipment slots (Primary, Secondary, Melee, Backpack)
- Active slot highlighting
- `renderHotbar()` - 6-slot hotbar (keys 4-9)
- Key number labels
- Total quantity display per slot

**4. Grid Layout**
- `GridLayout` struct for screen-space <-> grid-cell math
- `cellAt()` - Get grid cell from screen position
- `cellRect()` - Get screen rect for grid cell
- `contains()` - Check if point is within grid bounds

## Relationship to New UI Framework

The new `engine/ui/` framework (Phases 1-3) provides:
- General-purpose UI components (Button, Label, Image, Slider, Checkbox)
- Container system (VBox, HBox, Grid, SliderContainer, ScrollContainer)
- Theming system
- State management
- Layer management

The existing `game/ui/` system provides:
- Game-specific UI rendering
- Inventory drag-and-drop logic
- Equipment and hotbar display
- Grid mathematics

## Integration Strategy

The two systems can coexist:
1. **New UI Framework** - For menus, settings, new UI features
2. **Existing Game UI** - For inventory, equipment, hotbar (already working)

**Future Integration:**
- Use new UI components for settings menus (UISliderContainer)
- Use new UI framework for character sheet, journal, etc.
- Keep existing inventory/equipment system (it's already professional)
- Share `GridLayout` between both systems

## Recommendation

**DO NOT** extract the existing game UI into the new framework. The existing system is:
- ✅ Already working
- ✅ Well-structured
- ✅ Testable (dragdrop_test.cpp)
- ✅ Professional (drag-drop logic, grid math, rendering)
- ✅ Game-specific (unlike the general-purpose framework)

Instead, use the new UI framework for **new UI features** while keeping the existing inventory/equipment system intact.
