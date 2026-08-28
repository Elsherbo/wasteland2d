#pragma once

#include <optional>
#include <string>

#include "ecs/Registry.h"

#include "components/EquipmentSlots.h"
#include "components/Inventory.h"
#include "components/MeleeWeapon.h"
#include "components/Weapon.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"

namespace game::systems {

// Two genuinely separate concerns, both live here:
//  - equip()/unequip(): loadout management — assigning which item
//    occupies a weapon slot, done via the inventory UI. Moves the item
//    physically out of (or back into) the grid Inventory.
//  - syncActiveWeapon(): moment-to-moment combat readiness — copies
//    whichever slot is EquipmentSlots::activeSlot's stats onto the
//    entity's Weapon or MeleeWeapon component (removing the other),
//    so CombatSystem/MeleeCombatSystem's existing "does this entity
//    even have a Weapon/MeleeWeapon component" gate is the only thing
//    that needs to know whether firing/swinging is currently possible
//    — switching weapons (1/2/3, scroll) only ever calls this, it
//    never touches the inventory grid at all.
class EquipmentSystem {
public:
    // Carry-capacity constants for the Backpack slot's soft/hard weight
    // caps (see Inventory.h) — public so callers (main.cpp's initial
    // player setup, tests) can reason about the same numbers this class
    // uses internally, instead of duplicating them.
    static constexpr float kBaseCarryCapacityKg = 5.0f; // "pockets only" with no backpack equipped
    static constexpr float kOverloadMultiplier = 1.5f;  // hard cap = soft cap * this

    // Equips whatever item occupies (gridX, gridY) in inv into `slot`.
    // For Primary/Secondary/Melee: must be a weapon (has
    // ItemDefinition::weaponStats) of the matching kind
    // (Primary/Secondary -> Ranged, Melee -> Melee). For Backpack: must
    // have ItemDefinition::carryCapacityKg set, and a successful equip
    // immediately recomputes inv's soft/hard weight caps (see
    // applyCapacity) — which can leave inv already over its new hard
    // cap if you downgrade to a smaller backpack while full. That's
    // intentional, not a bug: swapping to a smaller bag while
    // overloaded should have a real consequence (see Encumbrance),
    // not silently rescue you from it. Every case also requires
    // maxStack == 1 (equip's "one item, one slot" model doesn't have a
    // sensible meaning for a stack of more than one — true of every
    // weapon and backpack in the current item data). Whatever was
    // previously in that slot (if anything) is returned to the grid,
    // first-fit — if there's no room for it, the whole equip is
    // refused (returns false) and nothing changes, rather than
    // silently destroying the previous item.
    static bool equip(components::Inventory& inv, components::EquipmentSlots& slots, const data::ItemDatabase& db,
                       components::EquipmentSlots::Slot slot, int gridX, int gridY) {
        auto idx = InventorySystem::stackIndexAt(inv, db, gridX, gridY);
        if (!idx) return false; // empty cell

        const auto& stack = inv.stacks[*idx];
        const data::ItemDefinition* def = db.find(stack.itemId);
        if (!def) return false;
        if (def->maxStack != 1) return false; // equip doesn't support stackable items

        bool isBackpackSlot = (slot == components::EquipmentSlots::Slot::Backpack);
        if (isBackpackSlot) {
            if (!def->carryCapacityKg.has_value()) return false; // not a backpack
        } else {
            if (!def->weaponStats.has_value()) return false; // not a weapon at all
            bool kindMatches = (slot == components::EquipmentSlots::Slot::Melee)
                                    ? (def->weaponStats->kind == data::WeaponKind::Melee)
                                    : (def->weaponStats->kind == data::WeaponKind::Ranged);
            if (!kindMatches) return false;
        }

        std::string newItemId = stack.itemId;
        float newCapacityKg = isBackpackSlot ? def->carryCapacityKg.value() : 0.0f;
        std::string& slotField = fieldFor(slots, slot);
        std::string previousItemId = slotField;

        auto removed = InventorySystem::removeStackAt(inv, db, gridX, gridY);
        if (!removed.has_value()) return false; // shouldn't happen — stackIndexAt just confirmed something's there

        if (!previousItemId.empty()) {
            int leftover = InventorySystem::addItem(inv, db, previousItemId, 1);
            if (leftover > 0) {
                // No room for the previously-equipped item — undo the
                // pickup and refuse the whole equip rather than lose it.
                inv.stacks.push_back(*removed);
                return false;
            }
        }

        slotField = newItemId;
        if (isBackpackSlot) applyCapacity(inv, newCapacityKg);
        return true;
    }

    // Removes whatever's equipped in `slot` and returns it to the
    // grid, first-fit. Returns false (slot stays equipped, nothing
    // lost) if the slot was already empty or the grid has no room.
    // Unequipping a backpack resets inv's weight caps to the base
    // (no-backpack) capacity — see applyCapacity.
    static bool unequip(components::Inventory& inv, components::EquipmentSlots& slots, const data::ItemDatabase& db,
                         components::EquipmentSlots::Slot slot) {
        std::string& slotField = fieldFor(slots, slot);
        if (slotField.empty()) return false;

        int leftover = InventorySystem::addItem(inv, db, slotField, 1);
        if (leftover > 0) return false;

        slotField.clear();
        if (slot == components::EquipmentSlots::Slot::Backpack) applyCapacity(inv, 0.0f);
        return true;
    }

    // Copies the currently-active slot's item stats onto entity's
    // Weapon or MeleeWeapon component — removing whichever one isn't
    // relevant, and removing both if the active slot is empty. Call
    // whenever activeSlot changes (switching), and after any
    // equip()/unequip() call that touched the currently-active slot
    // (an equip/unequip on a slot that ISN'T active doesn't need this
    // — that slot's stats simply aren't in effect yet).
    static void syncActiveWeapon(engine::ecs::Registry& registry, engine::ecs::Entity entity,
                                  const components::EquipmentSlots& slots, const data::ItemDatabase& db) {
        std::string activeItemId = itemIdFor(slots, slots.activeSlot);
        const data::ItemDefinition* def = activeItemId.empty() ? nullptr : db.find(activeItemId);
        bool hasMatchingStats = def && def->weaponStats.has_value();

        if (slots.activeSlot == components::EquipmentSlots::Slot::Melee) {
            if (registry.has<components::Weapon>(entity)) registry.remove<components::Weapon>(entity);

            if (hasMatchingStats && def->weaponStats->kind == data::WeaponKind::Melee) {
                components::MeleeWeapon melee;
                melee.damage = def->weaponStats->meleeDamage;
                melee.range = def->weaponStats->meleeRange;
                melee.arcDegrees = def->weaponStats->meleeArcDegrees;
                melee.attacksPerSecond = def->weaponStats->meleeAttacksPerSecond;
                melee.cooldown = 0.0f; // fresh — switching weapons doesn't carry over a leftover cooldown
                if (registry.has<components::MeleeWeapon>(entity)) {
                    registry.get<components::MeleeWeapon>(entity) = melee;
                } else {
                    registry.emplace<components::MeleeWeapon>(entity, melee);
                }
            } else if (registry.has<components::MeleeWeapon>(entity)) {
                registry.remove<components::MeleeWeapon>(entity); // melee slot empty — nothing to swing with
            }
        } else {
            if (registry.has<components::MeleeWeapon>(entity)) registry.remove<components::MeleeWeapon>(entity);

            if (hasMatchingStats && def->weaponStats->kind == data::WeaponKind::Ranged) {
                components::Weapon weapon;
                weapon.fireRate = def->weaponStats->fireRate;
                weapon.damage = def->weaponStats->damage;
                weapon.range = def->weaponStats->range;
                weapon.spreadDegrees = def->weaponStats->spreadDegrees;
                weapon.adsZoomMultiplier = def->weaponStats->adsZoomMultiplier;
                weapon.adsOffsetPixels = def->weaponStats->adsOffsetPixels;
                weapon.cooldown = 0.0f;
                if (registry.has<components::Weapon>(entity)) {
                    registry.get<components::Weapon>(entity) = weapon;
                } else {
                    registry.emplace<components::Weapon>(entity, weapon);
                }
            } else if (registry.has<components::Weapon>(entity)) {
                registry.remove<components::Weapon>(entity); // primary/secondary slot empty — nothing to fire
            }
        }
    }

private:
    static std::string& fieldFor(components::EquipmentSlots& slots, components::EquipmentSlots::Slot slot) {
        switch (slot) {
            case components::EquipmentSlots::Slot::Primary: return slots.primaryItemId;
            case components::EquipmentSlots::Slot::Secondary: return slots.secondaryItemId;
            case components::EquipmentSlots::Slot::Melee: return slots.meleeItemId;
            case components::EquipmentSlots::Slot::Backpack: return slots.backpackItemId;
        }
        return slots.primaryItemId; // unreachable — silences a missing-return warning
    }

    static std::string itemIdFor(const components::EquipmentSlots& slots, components::EquipmentSlots::Slot slot) {
        switch (slot) {
            case components::EquipmentSlots::Slot::Primary: return slots.primaryItemId;
            case components::EquipmentSlots::Slot::Secondary: return slots.secondaryItemId;
            case components::EquipmentSlots::Slot::Melee: return slots.meleeItemId;
            case components::EquipmentSlots::Slot::Backpack: return slots.backpackItemId;
        }
        return {};
    }

    // backpackCapacityKg = 0 means "no backpack" (base/pockets-only).
    // softMaxWeight is the stated capacity; maxWeight (the hard cap
    // InventorySystem::addItem actually enforces) is always
    // kOverloadMultiplier times that — see Encumbrance for how the gap
    // between the two becomes a movement penalty.
    static void applyCapacity(components::Inventory& inv, float backpackCapacityKg) {
        inv.softMaxWeight = kBaseCarryCapacityKg + backpackCapacityKg;
        inv.maxWeight = inv.softMaxWeight * kOverloadMultiplier;
    }
};

} // namespace game::systems
