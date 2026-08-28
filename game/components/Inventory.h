#pragma once

#include <string>
#include <vector>

namespace game::components {

// One placed stack of an item within an Inventory's grid — occupies a
// width x height footprint (from the item's ItemDefinition, looked up
// by itemId) starting at (gridX, gridY), top-left corner.
struct InventoryStack {
    std::string itemId;
    int quantity = 1;
    int gridX = 0;
    int gridY = 0;

    // true: this stack's effective footprint is its ItemDefinition's
    // width/height swapped (a 2x1 item becomes 1x2). Player-controlled
    // only (toggle while dragging — see DragDropController::
    // toggleRotation) — automatic placement (InventorySystem::addItem,
    // used by looting and corpse spawning) never sets this; it always
    // places at an item's natural, unrotated size. Defaults false, so
    // every existing stack/test that never touches this field behaves
    // exactly as before rotation support was added.
    bool rotated = false;
};

// Zero-Sievert/Tarkov-style shaped grid inventory: items occupy a
// rectangular footprint of cells rather than one item per slot.
// Placement, overlap-checking, stacking, and weight-limit logic all
// live in InventorySystem (game/systems/InventorySystem.h) — this
// component is deliberately just data, the same split CombatSystem
// uses between Health/Weapon (data) and itself (logic).
struct Inventory {
    int gridWidth = 6;
    int gridHeight = 4;

    // Two separate weight limits, not one — this is what makes the
    // DayZ/Zero-Sievert-style overload model possible: pickup is only
    // ever refused past maxWeight (the hard cap; InventorySystem's
    // gate is completely unchanged — it always checked exactly this
    // field). softMaxWeight is the *stated* capacity (a backpack's
    // number) — crossing it never blocks anything by itself, it's
    // purely a signal for movement-penalty math (see
    // EquipmentSystem::applyCapacity, systems::Encumbrance) to read.
    // Equal by default (no backpack-driven gap) — set explicitly by
    // EquipmentSystem when a backpack is equipped/unequipped.
    float softMaxWeight = 5.0f; // "pockets only" until a backpack changes this
    float maxWeight = 7.5f;

    std::vector<InventoryStack> stacks;
};

} // namespace game::components
