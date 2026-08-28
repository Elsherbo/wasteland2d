#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include "components/Inventory.h"
#include "data/ItemDatabase.h"

namespace game::systems {

// All placement/stacking/weight logic for the shaped-grid Inventory
// component lives here — Inventory itself stays plain data (see its
// own header for why).
class InventorySystem {
public:
    // True if a width x height footprint at (gridX, gridY) fits inside
    // the inventory's bounds and doesn't overlap any existing stack's
    // footprint (each looked up via db, since a stack only stores an
    // itemId, not its own footprint size — and each existing stack's
    // *effective* footprint accounts for InventoryStack::rotated, so a
    // rotated 2x1 item already placed correctly blocks a 1x2 area, not
    // its unrotated 2x1 one).
    static bool canPlace(const components::Inventory& inv, const data::ItemDatabase& db, int width,
                          int height, int gridX, int gridY) {
        if (width <= 0 || height <= 0) return false;
        if (gridX < 0 || gridY < 0 || gridX + width > inv.gridWidth || gridY + height > inv.gridHeight) {
            return false;
        }
        for (const auto& stack : inv.stacks) {
            const data::ItemDefinition* def = db.find(stack.itemId);
            if (!def) continue; // unknown item id — ignore rather than crash; addItem() never produces these
            int existingW = stack.rotated ? def->height : def->width;
            int existingH = stack.rotated ? def->width : def->height;
            bool overlapsX = gridX < stack.gridX + existingW && stack.gridX < gridX + width;
            bool overlapsY = gridY < stack.gridY + existingH && stack.gridY < gridY + height;
            if (overlapsX && overlapsY) return false;
        }
        return true;
    }

    // First free position (row-major scan: top row left-to-right, then
    // down) a width x height footprint could occupy, or std::nullopt
    // if nothing fits. Not optimal bin-packing — first fit, same
    // standard approach Tarkov-style inventories actually use.
    static std::optional<std::pair<int, int>> findFreePosition(const components::Inventory& inv,
                                                                 const data::ItemDatabase& db, int width,
                                                                 int height) {
        for (int y = 0; y <= inv.gridHeight - height; ++y) {
            for (int x = 0; x <= inv.gridWidth - width; ++x) {
                if (canPlace(inv, db, width, height, x, y)) {
                    return std::make_pair(x, y);
                }
            }
        }
        return std::nullopt;
    }

    static float currentWeight(const components::Inventory& inv, const data::ItemDatabase& db) {
        float total = 0.0f;
        for (const auto& stack : inv.stacks) {
            const data::ItemDefinition* def = db.find(stack.itemId);
            if (def) total += def->weight * static_cast<float>(stack.quantity);
        }
        return total;
    }

    // Adds up to `quantity` units of itemId to inv: first tops up
    // existing stacks of the same item that have room, then places new
    // stacks (each up to maxStack) at the first free grid position.
    // Stops early once the grid has no room left or maxWeight would be
    // exceeded. Returns how many units did NOT fit — 0 means everything
    // was added, `quantity` unchanged means nothing fit at all (grid
    // full, over weight, or unknown itemId).
    static int addItem(components::Inventory& inv, const data::ItemDatabase& db, const std::string& itemId,
                        int quantity) {
        const data::ItemDefinition* def = db.find(itemId);
        if (!def || quantity <= 0) return quantity;

        int remaining = quantity;
        // Computed once and tracked incrementally below (rather than
        // re-summing every stack on every iteration) — this keeps
        // addItem roughly O(n) in stack count instead of O(n^2), even
        // though at this inventory's scale (a handful to a few dozen
        // stacks) either would be fine in practice.
        float weightUsed = currentWeight(inv, db);

        auto weightBudgetUnits = [&]() -> int {
            if (def->weight <= 0.0f) return remaining; // weightless item — never budget-limited
            float budget = inv.maxWeight - weightUsed;
            if (budget <= 0.0f) return 0;
            // + a tiny epsilon before truncating: dividing decimal
            // weights (0.1, 0.7, ...) essentially never lands on an
            // exact integer in binary floating point, even when the
            // mathematically correct answer is one — e.g. (0.7 - 0.5)
            // / 0.1 evaluates to ~1.9999999999999996, not 2.0. Without
            // this, a weight budget that should fit exactly N units
            // undercounts to N-1, which reads to a player as "not
            // enough room" when there genuinely is exactly enough.
            return static_cast<int>(budget / def->weight + 1e-4f);
        };

        if (def->maxStack > 1) {
            for (auto& stack : inv.stacks) {
                if (remaining <= 0) break;
                if (stack.itemId != itemId) continue;
                int room = def->maxStack - stack.quantity;
                if (room <= 0) continue;

                int toAdd = std::min({room, remaining, weightBudgetUnits()});
                if (toAdd <= 0) {
                    if (weightBudgetUnits() <= 0) return remaining; // weight budget fully exhausted
                    continue; // this stack has no room, but budget remains — keep scanning others
                }

                stack.quantity += toAdd;
                remaining -= toAdd;
                weightUsed += def->weight * static_cast<float>(toAdd);
            }
        }

        while (remaining > 0) {
            int stackSize = std::min({def->maxStack, remaining, weightBudgetUnits()});
            if (stackSize <= 0) break; // no weight budget left for even one more unit

            auto pos = findFreePosition(inv, db, def->width, def->height);
            if (!pos.has_value()) break; // no grid space left

            components::InventoryStack newStack;
            newStack.itemId = itemId;
            newStack.quantity = stackSize;
            newStack.gridX = pos->first;
            newStack.gridY = pos->second;
            inv.stacks.push_back(newStack);

            remaining -= stackSize;
            weightUsed += def->weight * static_cast<float>(stackSize);
        }

        return remaining;
    }

    // Moves as much as possible from source into dest. Stacks that
    // fully move are removed from source; a stack that only partially
    // moves (dest ran out of grid space or weight budget partway) has
    // its quantity reduced but keeps its original grid position in
    // source — this is what gives "loot everything you can carry, the
    // corpse keeps the rest" behavior for free, rather than an
    // all-or-nothing transfer.
    static void moveAllTo(components::Inventory& source, components::Inventory& dest,
                           const data::ItemDatabase& db) {
        std::vector<components::InventoryStack> remaining;
        remaining.reserve(source.stacks.size());
        for (auto stack : source.stacks) {
            int leftover = addItem(dest, db, stack.itemId, stack.quantity);
            if (leftover > 0) {
                stack.quantity = leftover;
                remaining.push_back(stack);
            }
        }
        source.stacks = std::move(remaining);
    }

    // Index into inv.stacks of whichever stack occupies grid cell
    // (gridX, gridY) — accounting for InventoryStack::rotated — or
    // std::nullopt if the cell is empty. The one "which stack is here"
    // query, used by DragDropController's pickup, the quick-transfer
    // functions below, and hover-highlight rendering, so that logic
    // exists in exactly one place rather than being re-implemented at
    // each call site.
    static std::optional<std::size_t> stackIndexAt(const components::Inventory& inv, const data::ItemDatabase& db,
                                                     int gridX, int gridY) {
        for (std::size_t i = 0; i < inv.stacks.size(); ++i) {
            const auto& stack = inv.stacks[i];
            const data::ItemDefinition* def = db.find(stack.itemId);
            if (!def) continue;
            int w = stack.rotated ? def->height : def->width;
            int h = stack.rotated ? def->width : def->height;
            bool withinX = gridX >= stack.gridX && gridX < stack.gridX + w;
            bool withinY = gridY >= stack.gridY && gridY < stack.gridY + h;
            if (withinX && withinY) return i;
        }
        return std::nullopt;
    }

    // Removes and returns the stack occupying (gridX, gridY), or
    // std::nullopt if the cell is empty (inv is left unmodified in
    // that case).
    static std::optional<components::InventoryStack> removeStackAt(components::Inventory& inv,
                                                                     const data::ItemDatabase& db, int gridX,
                                                                     int gridY) {
        auto idx = stackIndexAt(inv, db, gridX, gridY);
        if (!idx) return std::nullopt;
        components::InventoryStack removed = inv.stacks[*idx];
        inv.stacks.erase(inv.stacks.begin() + static_cast<long>(*idx));
        return removed;
    }

    // Quick-grab/quick-store (Ctrl+click in the UI): moves the single
    // stack at (gridX, gridY) in `source` into `dest` via the normal
    // addItem first-fit/stacking rules. Whatever doesn't fit in dest
    // stays behind in source, at its original position, exactly like
    // moveAllTo's partial-transfer behavior — nothing is ever lost.
    // Returns false (source left completely untouched) if the cell was
    // empty, or if dest had room for none of it at all.
    //
    // Note: like addItem, this never preserves InventoryStack::rotated
    // — a rotated stack quick-transferred to dest is re-placed
    // unrotated there. Consistent with rotation being a purely manual,
    // drag-and-drop-only affordance (see Inventory.h) — every
    // *automatic* placement path (this one, addItem, moveAllTo) always
    // places at an item's natural size.
    static bool quickTransferStack(components::Inventory& source, components::Inventory& dest,
                                    const data::ItemDatabase& db, int gridX, int gridY) {
        auto idx = stackIndexAt(source, db, gridX, gridY);
        if (!idx) return false;

        components::InventoryStack stack = source.stacks[*idx];
        int leftover = addItem(dest, db, stack.itemId, stack.quantity);
        if (leftover == stack.quantity) return false; // nothing fit in dest — source left untouched

        source.stacks.erase(source.stacks.begin() + static_cast<long>(*idx));
        if (leftover > 0) {
            stack.quantity = leftover;
            source.stacks.push_back(stack); // whatever didn't fit stays behind, at its original position
        }
        return true;
    }

    // Index of the first stack (in storage order — not a meaningful
    // "best" choice, any matching stack is fungible with any other)
    // whose itemId matches, regardless of grid position — unlike
    // stackIndexAt, which finds whatever's at a specific cell. Used by
    // hotbar quick-slot use (UseItemSystem): a quick slot binds to an
    // itemId, not a specific grid position, since consumables move
    // around the grid as they stack/split.
    static std::optional<std::size_t> findAnyStackOf(const components::Inventory& inv, const std::string& itemId) {
        for (std::size_t i = 0; i < inv.stacks.size(); ++i) {
            if (inv.stacks[i].itemId == itemId) return i;
        }
        return std::nullopt;
    }

    // Total quantity of itemId across every stack in inv, summed —
    // for display (a hotbar slot showing "12" total bandages even if
    // they're split across two grid stacks of 5+7), not for placement
    // logic.
    static int totalQuantityOf(const components::Inventory& inv, const std::string& itemId) {
        int total = 0;
        for (const auto& stack : inv.stacks) {
            if (stack.itemId == itemId) total += stack.quantity;
        }
        return total;
    }
};

} // namespace game::systems
