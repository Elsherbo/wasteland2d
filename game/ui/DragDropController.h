#pragma once

#include <algorithm>
#include <optional>
#include <utility>

#include "components/Inventory.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"

namespace game::ui {

// A picked-up, mid-drag item stack — not committed to any inventory
// until endDrag() places it (or cancelDrag()/an invalid drop returns
// it to where it came from). stack.gridX/gridY double as "original
// position" for the return-on-cancel path, right up until a successful
// placement overwrites them. stack.rotated is the only place rotation
// state lives during a drag — toggleRotation() flips it directly.
//
// grabOffsetX/Y: which cell within the item's footprint was actually
// clicked, relative to its top-left corner (0,0 = grabbed by its own
// top-left; for a 2x2 item, 1,1 = grabbed by its bottom-right cell).
// This is what lets the item track the cursor at the point you actually
// grabbed it instead of always snapping its top-left corner under the
// mouse — see resolveDropTopLeft().
struct HeldStack {
    bool active = false;
    components::InventoryStack stack;
    components::Inventory* sourceInventory = nullptr;
    int grabOffsetX = 0;
    int grabOffsetY = 0;
};

// What a drop at a given position would do, if committed — see
// planDrop()'s doc comment for why this exists as its own type rather
// than endDrag() just deciding inline.
enum class DropOutcome {
    Invalid, // nothing here would work — a real endDrag() call falls back to returning the stack to its source
    Place,   // footprint is fully clear and the weight budget allows a brand new stack here
    Merge,   // lands on an existing same-item stack, at its exact position, with room (fully or partially)
    Swap,    // lands on exactly one different-item stack whose footprint exactly matches — positions trade places
};

// This is game-level, not engine/, because it operates directly on
// game::components::Inventory and game::data::ItemDatabase — the same
// reasoning that keeps InventorySystem and CombatSystem in game/
// despite being "reusable-ish" concepts (see InventorySystem.h). A
// genuinely generic, abstracted engine::ui drag-and-drop framework
// would need to guess at what a different game's grid UI wants without
// any second project to validate that guess against — premature
// abstraction, not a shortcut. What's actually engine-level here
// (GridLayout, GridRenderer) already is.
class DragDropController {
public:
    // Picks up whatever stack (if any) occupies (gridX, gridY) within
    // inventory, recording that cell as the "grab point" within the
    // item's footprint (see HeldStack). No-op (returns false) if
    // already holding something, or the cell is empty.
    bool beginDrag(const data::ItemDatabase& db, components::Inventory& inventory, int gridX, int gridY) {
        if (held_.active) return false;

        auto idx = systems::InventorySystem::stackIndexAt(inventory, db, gridX, gridY);
        if (!idx) return false; // empty cell — nothing to pick up

        held_.active = true;
        held_.stack = inventory.stacks[*idx]; // gridX/gridY/rotated here = original state, kept for a possible cancel
        held_.sourceInventory = &inventory;
        held_.grabOffsetX = gridX - held_.stack.gridX;
        held_.grabOffsetY = gridY - held_.stack.gridY;
        inventory.stacks.erase(inventory.stacks.begin() + static_cast<long>(*idx));
        return true;
    }

    // Given the grid cell the mouse currently hovers over, returns the
    // cell that would become the held item's top-left corner if
    // dropped here — i.e. hoverCell minus the grab offset recorded in
    // beginDrag(). This is the one place that conversion happens;
    // previewDrop()/endDrag() both take an already-resolved top-left
    // (not a raw hover cell) precisely so a caller can use this same
    // resolved position for both the highlight/ghost rendering and the
    // actual drop decision, and never have them disagree. Returns the
    // input unchanged if nothing is currently held.
    std::pair<int, int> resolveDropTopLeft(int hoverGridX, int hoverGridY) const {
        if (!held_.active) return {hoverGridX, hoverGridY};
        return {hoverGridX - held_.grabOffsetX, hoverGridY - held_.grabOffsetY};
    }

    // Rotates the held stack (swaps its effective width/height) —
    // no-op if nothing is currently held. Purely a drag-time toggle;
    // see Inventory.h for why automatic placement never rotates
    // anything on its own. The grab offset's axes swap along with the
    // footprint (this isn't a true geometric rotation of the grab
    // point — just the same axis-swap the dimensions themselves get —
    // but it keeps the offset valid within the new footprint bounds by
    // construction, and there's no visual spin to match anyway, only a
    // reinterpreted orientation).
    void toggleRotation() {
        if (!held_.active) return;
        held_.stack.rotated = !held_.stack.rotated;
        std::swap(held_.grabOffsetX, held_.grabOffsetY);
    }

    // What a drop at (gridX, gridY) into targetInventory would do
    // right now, without changing anything — Invalid if nothing is
    // currently held. This is exactly the same decision endDrag() acts
    // on (it calls this internally), so the UI's drop-preview
    // highlight and the real drop can never disagree with each other —
    // there is only one place this decision gets made.
    DropOutcome previewDrop(const data::ItemDatabase& db, const components::Inventory& targetInventory, int gridX,
                             int gridY) const {
        return planDrop(db, targetInventory, gridX, gridY).outcome;
    }

    // Commits a drop at (gridX, gridY) into targetInventory, per
    // previewDrop()'s outcome for the same position — merges, swaps,
    // places fresh, or (Invalid) returns the stack to its source at
    // its original position, same as cancelDrag(). No-op if nothing is
    // currently held.
    void endDrag(const data::ItemDatabase& db, components::Inventory& targetInventory, int gridX, int gridY) {
        if (!held_.active) return;

        DropPlan plan = planDrop(db, targetInventory, gridX, gridY);
        switch (plan.outcome) {
            case DropOutcome::Place: {
                held_.stack.gridX = gridX;
                held_.stack.gridY = gridY;
                targetInventory.stacks.push_back(held_.stack);
                held_.active = false;
                return;
            }
            case DropOutcome::Merge: {
                auto& existing = targetInventory.stacks[*plan.targetStackIndex];
                existing.quantity += plan.mergeAmount;
                held_.stack.quantity -= plan.mergeAmount;
                if (held_.stack.quantity <= 0) {
                    held_.active = false; // fully merged
                } else {
                    // Partial merge: whatever's left keeps its original
                    // gridX/gridY (never touched in this branch), so
                    // cancelDrag() below correctly returns just the
                    // leftover, not the full original quantity.
                    cancelDrag();
                }
                return;
            }
            case DropOutcome::Swap: {
                components::InventoryStack blocking = targetInventory.stacks[*plan.targetStackIndex];
                components::InventoryStack movedBack = blocking;
                movedBack.gridX = held_.stack.gridX; // held_.stack's ORIGINAL position, not yet overwritten
                movedBack.gridY = held_.stack.gridY;
                movedBack.rotated = false; // returning to its footprint-matching original slot — no rotation needed

                targetInventory.stacks.erase(targetInventory.stacks.begin() +
                                              static_cast<long>(*plan.targetStackIndex));

                held_.stack.gridX = gridX;
                held_.stack.gridY = gridY;
                targetInventory.stacks.push_back(held_.stack);
                held_.sourceInventory->stacks.push_back(movedBack);

                held_.active = false;
                return;
            }
            case DropOutcome::Invalid:
                cancelDrag();
                return;
        }
    }

    // Returns the held stack (or whatever's left of it, after a
    // partial merge) to its source inventory, at its original
    // position. Always succeeds — that position was valid before
    // pickup and nothing else has touched it since. No-op if nothing
    // is currently held.
    void cancelDrag() {
        if (!held_.active) return;
        if (held_.sourceInventory) {
            held_.sourceInventory->stacks.push_back(held_.stack);
        }
        held_.active = false;
    }

    bool isDragging() const { return held_.active; }
    const HeldStack& held() const { return held_; }

private:
    struct DropPlan {
        DropOutcome outcome = DropOutcome::Invalid;
        std::optional<std::size_t> targetStackIndex; // for Merge or Swap
        int mergeAmount = 0;                          // for Merge only
    };

    // The single source of truth for "what would happen if the held
    // stack were dropped at (gridX, gridY) in targetInventory" — read
    // this, don't reimplement it. Tries, in order: merge onto an
    // exact-position same-item stack with room; swap with exactly one
    // different-item stack whose footprint exactly matches; a fresh
    // placement into fully clear space; Invalid otherwise.
    DropPlan planDrop(const data::ItemDatabase& db, const components::Inventory& targetInventory, int gridX,
                       int gridY) const {
        DropPlan plan;
        if (!held_.active) return plan;

        const data::ItemDefinition* def = db.find(held_.stack.itemId);
        if (!def) return plan; // unknown item id — shouldn't happen; fail safe rather than guess

        int heldW = held_.stack.rotated ? def->height : def->width;
        int heldH = held_.stack.rotated ? def->width : def->height;

        // 1) Merge: an existing stack of the same item, at exactly
        //    this position, with room and weight budget for at least
        //    one more unit.
        for (std::size_t i = 0; i < targetInventory.stacks.size(); ++i) {
            const auto& existing = targetInventory.stacks[i];
            if (existing.itemId != held_.stack.itemId || existing.gridX != gridX || existing.gridY != gridY) {
                continue;
            }
            int room = def->maxStack - existing.quantity;
            if (room <= 0) break; // full — an exact-position same-item stack with no room blocks placement entirely

            float budget = targetInventory.maxWeight - systems::InventorySystem::currentWeight(targetInventory, db);
            int affordable =
                def->weight > 0.0f ? static_cast<int>(budget / def->weight + 1e-4f) : held_.stack.quantity;
            int toMerge = std::min({room, held_.stack.quantity, std::max(0, affordable)});
            if (toMerge <= 0) break; // no weight budget for even one more unit

            plan.outcome = DropOutcome::Merge;
            plan.targetStackIndex = i;
            plan.mergeAmount = toMerge;
            return plan;
        }

        // 2) Swap: the drop footprint overlaps exactly one existing
        //    stack (a different item), whose own footprint exactly
        //    matches the position and size the held item would occupy
        //    here. Deliberately restricted to an exact match — a held
        //    item that's a different size than what's underneath it is
        //    treated as a normal (colliding, thus invalid) placement
        //    below, not a resize-and-swap.
        std::optional<std::size_t> blockingIndex;
        bool exactlyOneBlocker = true;
        for (std::size_t i = 0; i < targetInventory.stacks.size(); ++i) {
            const auto& existing = targetInventory.stacks[i];
            const data::ItemDefinition* existingDef = db.find(existing.itemId);
            if (!existingDef) continue;
            int ew = existing.rotated ? existingDef->height : existingDef->width;
            int eh = existing.rotated ? existingDef->width : existingDef->height;
            bool overlapsX = gridX < existing.gridX + ew && existing.gridX < gridX + heldW;
            bool overlapsY = gridY < existing.gridY + eh && existing.gridY < gridY + heldH;
            if (overlapsX && overlapsY) {
                if (blockingIndex.has_value()) {
                    exactlyOneBlocker = false;
                    break;
                }
                blockingIndex = i;
            }
        }

        if (blockingIndex.has_value() && exactlyOneBlocker) {
            const auto& blocking = targetInventory.stacks[*blockingIndex];
            if (blocking.itemId != held_.stack.itemId) {
                const data::ItemDefinition* blockingDef = db.find(blocking.itemId);
                if (blockingDef) {
                    int bw = blocking.rotated ? blockingDef->height : blockingDef->width;
                    int bh = blocking.rotated ? blockingDef->width : blockingDef->height;
                    bool exactFootprintMatch =
                        (blocking.gridX == gridX && blocking.gridY == gridY && bw == heldW && bh == heldH);

                    if (exactFootprintMatch && held_.sourceInventory) {
                        // The blocking stack needs to fit back at the
                        // held item's ORIGINAL position, in its ORIGINAL
                        // (source) inventory. Grid-wise it must — that's
                        // exactly where a same-sized item just came
                        // from — but the two inventories could have
                        // different weight budgets (a swap between the
                        // player and a corpse, say), so that still needs
                        // checking before this is safe to commit to.
                        float sourceBudget = held_.sourceInventory->maxWeight -
                                              systems::InventorySystem::currentWeight(*held_.sourceInventory, db);
                        float blockingWeight = blockingDef->weight * static_cast<float>(blocking.quantity);
                        if (blockingWeight <= sourceBudget + 1e-4f) {
                            plan.outcome = DropOutcome::Swap;
                            plan.targetStackIndex = blockingIndex;
                            return plan;
                        }
                    }
                }
            }
        }

        // 3) Plain placement: footprint fully clear, weight budget allows.
        if (systems::InventorySystem::canPlace(targetInventory, db, heldW, heldH, gridX, gridY)) {
            float weightIfPlaced = systems::InventorySystem::currentWeight(targetInventory, db) +
                                    def->weight * static_cast<float>(held_.stack.quantity);
            if (weightIfPlaced <= targetInventory.maxWeight + 1e-4f) {
                plan.outcome = DropOutcome::Place;
                return plan;
            }
        }

        return plan; // Invalid
    }

    HeldStack held_;
};

} // namespace game::ui
