#pragma once

#include <string>

namespace game::components {

// Which of an entity's three weapon slots is currently in-hand/active
// for firing — switching between them (1/2/3 keys, scroll wheel) only
// ever changes this, and is a completely different action from
// *equipping* something into a slot in the first place (dragging/
// assigning an item from the inventory grid — see EquipmentSystem).
struct EquipmentSlots {
    enum class Slot { Primary, Secondary, Melee, Backpack };

    // Empty string = nothing equipped in that slot. Primary/Secondary
    // must reference an item whose WeaponStats::kind is Ranged; Melee
    // must reference one whose kind is Melee; Backpack must reference
    // an item with ItemDefinition::carryCapacityKg set —
    // EquipmentSystem::equip enforces this, this struct itself doesn't
    // validate anything.
    std::string primaryItemId;
    std::string secondaryItemId;
    std::string meleeItemId;
    std::string backpackItemId;

    // Which of Primary/Secondary/Melee is in-hand right now — Backpack
    // is never "active"; it's always worn, there's nothing to switch
    // to. See EquipmentSystem::syncActiveWeapon.
    Slot activeSlot = Slot::Primary;
};

} // namespace game::components
