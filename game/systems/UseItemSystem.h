#pragma once

#include "ecs/Registry.h"

#include "components/Health.h"
#include "components/Inventory.h"
#include "components/QuickSlots.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"

namespace game::systems {

struct UseResult {
    bool used = false;        // false: slot unassigned, item not found in inventory, or item has no UseEffect
    float healedAmount = 0.0f; // actual Health delta applied, after clamping to max — 0 if nothing to heal
};

// Consuming a hotbar-bound item — a bandage restoring Health, for
// instance. Deliberately instant (no application/channel time): a real
// "takes a few seconds to use, can be interrupted" mechanic is a
// genuinely separate feature (needs a timer, a way to cancel it, a
// visual for it), not attempted here — MeleeWeapon::swingDuration has
// the same kind of "the field exists, nothing animates it yet" gap for
// the same reason: scoping the mechanic before the polish.
class UseItemSystem {
public:
    // slotIndex is 0-5 (key 4 through key 9 — see QuickSlots). No-op
    // (returns {false, 0}) for an out-of-range index, an unassigned
    // slot, an item with no stock left in inv, or an item with no
    // UseEffect at all.
    static UseResult useQuickSlot(engine::ecs::Registry& registry, engine::ecs::Entity entity,
                                   components::Inventory& inv, const components::QuickSlots& quickSlots,
                                   const data::ItemDatabase& db, int slotIndex) {
        UseResult result;
        if (slotIndex < 0 || slotIndex >= components::QuickSlots::kSlotCount) return result;

        const std::string& itemId = quickSlots.itemIds[static_cast<std::size_t>(slotIndex)];
        if (itemId.empty()) return result;

        const data::ItemDefinition* def = db.find(itemId);
        if (!def || !def->useEffect.has_value()) return result;

        auto idx = InventorySystem::findAnyStackOf(inv, itemId);
        if (!idx) return result; // none left

        // Only consume the item if applying its effect was even
        // possible — using a bandage on a dead (or Health-less) entity
        // shouldn't burn the item for no effect. Using one while
        // already at full health is different: that's a real, if
        // wasteful, use (healedAmount just comes out as 0 after
        // clamping) — most games let you make that mistake rather than
        // silently protecting you from it, and this does too.
        if (def->useEffect->healAmount > 0.0f) {
            if (!registry.has<components::Health>(entity)) return result;
            auto& health = registry.get<components::Health>(entity);
            if (health.dead) return result;

            float before = health.current;
            health.current += def->useEffect->healAmount;
            if (health.current > health.max) health.current = health.max;
            result.healedAmount = health.current - before; // actual delta, after clamping
        }

        auto& stack = inv.stacks[*idx];
        stack.quantity -= 1;
        if (stack.quantity <= 0) {
            inv.stacks.erase(inv.stacks.begin() + static_cast<long>(*idx));
        }

        result.used = true;
        return result;
    }
};

} // namespace game::systems
