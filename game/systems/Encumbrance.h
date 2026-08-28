#pragma once

#include "components/Inventory.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"

namespace game::systems {

// The movement half of the overload model — EquipmentSystem sets an
// Inventory's soft/hard weight caps (via backpack equip/unequip); this
// is what turns "current weight relative to those caps" into an actual
// speed multiplier. Kept separate from EquipmentSystem/InventorySystem
// on purpose: this is about movement, not placement or loadout —
// mixing it into either would blur what those classes are for.
class Encumbrance {
public:
    // Never truly 0 — a fully-overloaded character can still crawl at
    // this fraction of normal speed, rather than appearing frozen
    // (which reads as a bug, not a game mechanic) once at/past the
    // hard cap.
    static constexpr float kMinSpeedMultiplier = 0.05f;

    // 1.0 at or under softMaxWeight (no penalty at all — this is the
    // backpack's stated capacity, not a soft warning threshold).
    // Linearly interpolates down to kMinSpeedMultiplier as weight
    // climbs from softMaxWeight to maxWeight (the hard cap
    // InventorySystem::addItem actually enforces — weight can
    // approach it but picking up more is refused once there's no
    // budget left, so this rarely reaches maxWeight exactly, only
    // gets close). Stays clamped at kMinSpeedMultiplier for any
    // weight at or beyond maxWeight, including the case where a
    // backpack downgrade (see EquipmentSystem::equip) drops the caps
    // below what's already being carried.
    static float speedMultiplier(const components::Inventory& inv, const data::ItemDatabase& db) {
        float weight = InventorySystem::currentWeight(inv, db);
        if (weight <= inv.softMaxWeight) return 1.0f;

        float range = inv.maxWeight - inv.softMaxWeight;
        if (range <= 0.0f) return kMinSpeedMultiplier; // degenerate configuration (hard cap <= soft cap) — fail safe

        float t = (weight - inv.softMaxWeight) / range;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        return 1.0f - t * (1.0f - kMinSpeedMultiplier);
    }
};

} // namespace game::systems
