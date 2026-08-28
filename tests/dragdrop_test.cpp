// Standalone test of GridLayout (no dependencies at all) and
// DragDropController (needs ItemDatabase/nlohmann::json, nothing else —
// no SDL, no Box2D, since drag/drop logic itself never touches
// rendering or physics).
#include <cassert>
#include <cstdio>
#include <fstream>

#include "ui/GridLayout.h"

#include "components/Inventory.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"
#include "ui/DragDropController.h"

using engine::ui::GridLayout;
using game::components::Inventory;
using game::ui::DragDropController;
using game::ui::DropOutcome;

namespace {
void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([
        {"id": "arrow", "name": "Arrow", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "ammo"},
        {"id": "crate", "name": "Crate", "width": 2, "height": 2, "maxStack": 1, "weight": 5.0, "category": "misc"},
        {"id": "toolbox", "name": "Toolbox", "width": 2, "height": 2, "maxStack": 1, "weight": 3.0, "category": "misc"},
        {"id": "sword", "name": "Sword", "width": 1, "height": 3, "maxStack": 1, "weight": 2.0, "category": "weapon"}
    ])";
}
} // namespace

int main() {
    // --- GridLayout: pure screen<->cell math ---
    {
        GridLayout layout;
        layout.screenX = 100;
        layout.screenY = 50;
        layout.cellSize = 32;
        layout.gridWidth = 4;
        layout.gridHeight = 3;

        auto cell = layout.cellAt(100 + 32 * 2 + 5, 50 + 32 * 1 + 10); // inside cell (2,1)
        assert(cell.has_value());
        assert(cell->first == 2 && cell->second == 1);

        assert(!layout.cellAt(50, 50).has_value());   // left of the grid entirely
        assert(!layout.cellAt(100, 10).has_value());  // above the grid entirely
        assert(!layout.cellAt(100 + 32 * 4, 50).has_value()); // one cell past the right edge
        assert(!layout.cellAt(100, 50 + 32 * 3).has_value()); // one cell past the bottom edge

        int px = 0, py = 0, pw = 0, ph = 0;
        layout.cellRect(2, 1, px, py, pw, ph);
        assert(px == 100 + 32 * 2 && py == 50 + 32 * 1 && pw == 32 && ph == 32);

        std::printf("[ok] GridLayout: cellAt/cellRect correct at bounds and inside\n");
    }

    // --- DragDropController ---
    const std::string dbPath = "/tmp/dragdrop_test_items.json";
    writeTestItemDatabase(dbPath);
    game::data::ItemDatabase db;
    db.loadFromFile(dbPath);

    // Pickup: clicking anywhere within a shaped item's footprint (not
    // just its origin cell) must pick it up.
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "crate", 1); // lands at (0,0), footprint (0,0)-(1,1)

        DragDropController drag;
        bool picked = drag.beginDrag(db, inv, 1, 1); // bottom-right cell of the crate's footprint, not its origin
        assert(picked);
        assert(drag.isDragging());
        assert(inv.stacks.empty()); // removed from the inventory while held
        assert(drag.held().stack.itemId == "crate");
        std::printf("[ok] beginDrag: picked up via a non-origin cell within the item's footprint\n");

        // A second beginDrag while already holding something must be a no-op.
        Inventory otherInv;
        bool pickedAgain = drag.beginDrag(db, otherInv, 0, 0);
        assert(!pickedAgain);
        std::printf("[ok] beginDrag: correctly refuses a second pickup while already dragging\n");

        drag.cancelDrag();
    }

    // Pickup on an empty cell: no-op.
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        DragDropController drag;
        assert(!drag.beginDrag(db, inv, 0, 0));
        assert(!drag.isDragging());
        std::printf("[ok] beginDrag: empty cell correctly rejected\n");
    }

    // Successful placement into empty space.
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "crate", 1); // at (0,0)

        DragDropController drag;
        drag.beginDrag(db, inv, 0, 0);
        drag.endDrag(db, inv, 2, 2); // move it elsewhere within the same inventory

        assert(!drag.isDragging());
        assert(inv.stacks.size() == 1);
        assert(inv.stacks[0].gridX == 2 && inv.stacks[0].gridY == 2);
        std::printf("[ok] endDrag: moved to a new, empty position\n");
    }

    // Invalid drop (collides with something else) -> returned to source
    // at its original position, nothing lost.
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "crate", 1); // crate A at (0,0)
        game::systems::InventorySystem::addItem(inv, db, "crate", 1); // crate B at (2,0)

        DragDropController drag;
        drag.beginDrag(db, inv, 0, 0);      // pick up crate A
        drag.endDrag(db, inv, 2, 0);        // try to drop it directly onto crate B -> invalid (occupied, different item)

        assert(!drag.isDragging());
        assert(inv.stacks.size() == 2);
        bool foundOriginal = false;
        for (const auto& s : inv.stacks) {
            if (s.itemId == "crate" && s.gridX == 0 && s.gridY == 0) foundOriginal = true;
        }
        assert(foundOriginal); // crate A landed back exactly where it started
        std::printf("[ok] endDrag: invalid drop correctly returned the stack to its original position\n");
    }

    // Merge: dropping onto an existing same-item stack at its exact
    // position, with room, combines them instead of rejecting.
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "arrow", 3); // one stack of 3 at (0,0)
        game::systems::InventorySystem::addItem(inv, db, "arrow", 2); // second stack of 2 at (1,0)

        DragDropController drag;
        drag.beginDrag(db, inv, 1, 0);   // pick up the stack of 2
        drag.endDrag(db, inv, 0, 0);     // drop directly onto the stack of 3 (maxStack 5 -> room for 2 more)

        assert(!drag.isDragging());
        assert(inv.stacks.size() == 1); // merged into one stack
        assert(inv.stacks[0].quantity == 5);
        assert(inv.stacks[0].gridX == 0 && inv.stacks[0].gridY == 0);
        std::printf("[ok] endDrag: merged onto an existing same-item stack (3 + 2 -> 5)\n");
    }

    // Partial merge: target stack doesn't have room for all of it ->
    // remainder is returned to source, nothing lost or duplicated.
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "arrow", 4); // stack of 4 (maxStack 5, room for 1) at (0,0)
        game::systems::InventorySystem::addItem(inv, db, "arrow", 3); // stack of 3 at (1,0)

        DragDropController drag;
        drag.beginDrag(db, inv, 1, 0);   // pick up the stack of 3
        drag.endDrag(db, inv, 0, 0);     // drop onto the stack of 4 -> only 1 fits, 2 must come back

        assert(!drag.isDragging());
        assert(inv.stacks.size() == 2);
        int total = 0;
        bool foundReturned = false;
        for (const auto& s : inv.stacks) {
            total += s.quantity;
            if (s.gridX == 1 && s.gridY == 0) {
                foundReturned = true;
                assert(s.quantity == 2); // the leftover that didn't merge, back at its original slot
            }
        }
        assert(foundReturned);
        assert(total == 7); // 4 + 3 conserved across the partial merge
        std::printf("[ok] endDrag: partial merge -- 1 of 3 merged, 2 correctly returned to source\n");
    }

    // --- rotation: changes whether a shaped item fits ---
    {
        Inventory staging; // holds the sword unrotated first, so beginDrag has somewhere real to pick it up from
        staging.gridWidth = 1;
        staging.gridHeight = 3;
        staging.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(staging, db, "sword", 1); // 1x3, fits exactly at (0,0)

        Inventory target; // a single row — a 1x3 sword can ONLY fit here rotated (3x1)
        target.gridWidth = 3;
        target.gridHeight = 1;
        target.maxWeight = 1000.0f;

        DragDropController drag;
        drag.beginDrag(db, staging, 0, 0);
        assert(drag.isDragging());

        assert(drag.previewDrop(db, target, 0, 0) == DropOutcome::Invalid); // unrotated 1x3 doesn't fit a 1-row grid
        std::printf("[ok] rotation: unrotated 1x3 item correctly can't fit a 1x1-tall grid\n");

        drag.toggleRotation();
        assert(drag.previewDrop(db, target, 0, 0) == DropOutcome::Place); // rotated -> 3x1, fits exactly
        std::printf("[ok] rotation: same item, rotated to 3x1, now fits -- preview says Place\n");

        drag.endDrag(db, target, 0, 0);
        assert(!drag.isDragging());
        assert(target.stacks.size() == 1);
        assert(target.stacks[0].rotated);
        assert(target.stacks[0].gridX == 0 && target.stacks[0].gridY == 0);
        std::printf("[ok] rotation: endDrag committed the rotated placement, matching the preview\n");
    }

    // --- swap: two different items, exact-matching footprints, across two different inventories ---
    {
        Inventory source;
        source.gridWidth = 4;
        source.gridHeight = 4;
        source.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(source, db, "crate", 1); // at (0,0), 2x2

        Inventory dest;
        dest.gridWidth = 4;
        dest.gridHeight = 4;
        dest.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(dest, db, "toolbox", 1); // at (0,0), 2x2 -- same footprint, different item

        DragDropController drag;
        drag.beginDrag(db, source, 0, 0); // pick up the crate
        assert(drag.previewDrop(db, dest, 0, 0) == DropOutcome::Swap);

        drag.endDrag(db, dest, 0, 0);
        assert(!drag.isDragging());

        assert(dest.stacks.size() == 1);
        assert(dest.stacks[0].itemId == "crate" && dest.stacks[0].gridX == 0 && dest.stacks[0].gridY == 0);
        assert(source.stacks.size() == 1);
        assert(source.stacks[0].itemId == "toolbox" && source.stacks[0].gridX == 0 && source.stacks[0].gridY == 0);
        std::printf("[ok] swap (cross-inventory): crate <-> toolbox traded places between source and dest\n");
    }

    // --- swap: same inventory (reorganizing, not looting) ---
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "crate", 1);   // at (0,0)
        game::systems::InventorySystem::addItem(inv, db, "toolbox", 1); // at (2,0) -- first free spot after the crate

        DragDropController drag;
        drag.beginDrag(db, inv, 0, 0); // pick up the crate from (0,0)
        assert(drag.previewDrop(db, inv, 2, 0) == DropOutcome::Swap);

        drag.endDrag(db, inv, 2, 0);
        assert(!drag.isDragging());

        assert(inv.stacks.size() == 2);
        bool crateAt2 = false, toolboxAt0 = false;
        for (const auto& s : inv.stacks) {
            if (s.itemId == "crate" && s.gridX == 2 && s.gridY == 0) crateAt2 = true;
            if (s.itemId == "toolbox" && s.gridX == 0 && s.gridY == 0) toolboxAt0 = true;
        }
        assert(crateAt2 && toolboxAt0);
        std::printf("[ok] swap (same inventory): crate and toolbox correctly traded positions in place\n");
    }

    // --- grab-point offset: picking up a shaped item off-center keeps
    //     that same relative point under the cursor, not the item's
    //     top-left corner ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 6;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "crate", 1); // 2x2 at (0,0)

        DragDropController drag;
        drag.beginDrag(db, inv, 1, 1); // clicked the crate's bottom-right cell, not its origin

        // Hovering over cell (5,5) should resolve to top-left (4,4) --
        // i.e. keep the same cell-1,1-within-the-item under the cursor,
        // not snap the item's corner to (5,5).
        auto [resolvedX, resolvedY] = drag.resolveDropTopLeft(5, 5);
        assert(resolvedX == 4 && resolvedY == 4);
        std::printf("[ok] resolveDropTopLeft: grabbed off-center, resolves with the grab offset preserved\n");

        drag.endDrag(db, inv, resolvedX, resolvedY);
        assert(!drag.isDragging());
        assert(inv.stacks.size() == 1);
        assert(inv.stacks[0].gridX == 4 && inv.stacks[0].gridY == 4);
        std::printf("[ok] resolveDropTopLeft -> endDrag: item actually lands where the preview said it would\n");
    }

    // --- grab offset survives rotation (axes swap along with the footprint) ---
    {
        Inventory inv;
        inv.gridWidth = 1;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "sword", 1); // 1x3 at (0,0)

        DragDropController drag;
        drag.beginDrag(db, inv, 0, 1); // grabbed the middle cell -> offset (0,1)

        drag.toggleRotation(); // footprint becomes 3x1; offset swaps to (1,0)
        auto [resolvedX, resolvedY] = drag.resolveDropTopLeft(5, 5);
        assert(resolvedX == 4 && resolvedY == 5); // 5 - 1 (swapped offsetX), 5 - 0 (swapped offsetY)
        std::printf("[ok] resolveDropTopLeft: grab offset axes swap correctly along with rotation\n");
    }

    std::printf("ALL DRAGDROP TESTS PASSED\n");
    return 0;
}
