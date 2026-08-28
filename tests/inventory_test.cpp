// Standalone test of InventorySystem: no SDL, no Box2D — needs
// nlohmann::json (for ItemDatabase) and nothing else engine-side.
// Writes its own small item database to a temp file rather than
// depending on assets/items/items.json's exact contents, so this test
// stays correct even if that file's content changes later.
#include <cassert>
#include <cstdio>
#include <fstream>

#include "components/Inventory.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"
#include "test_common.h"

using game::components::Inventory;
using game::systems::InventorySystem;

namespace {

void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([
        {"id": "arrow", "name": "Arrow", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "ammo"},
        {"id": "crate", "name": "Crate", "width": 2, "height": 2, "maxStack": 1, "weight": 5.0, "category": "misc"}
    ])";
}

} // namespace

int main() {
    const std::string dbPath = getTestTempPath("inventory_test_items.json");
    writeTestItemDatabase(dbPath);

    game::data::ItemDatabase db;
    db.loadFromFile(dbPath);
    assert(db.find("arrow") != nullptr);
    assert(db.find("crate") != nullptr);
    assert(db.find("nonexistent") == nullptr);
    std::printf("[ok] item database loaded\n");

    // --- stacking: 12 arrows (maxStack 5) into an empty 4x4 grid ---
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f; // not the constraint being tested here

        int leftover = InventorySystem::addItem(inv, db, "arrow", 12);
        assert(leftover == 0);
        assert(inv.stacks.size() == 3); // 5 + 5 + 2
        assert(inv.stacks[0].quantity == 5);
        assert(inv.stacks[1].quantity == 5);
        assert(inv.stacks[2].quantity == 2);
        std::printf("[ok] stacking: 12 arrows -> 3 stacks (5, 5, 2), 0 leftover\n");
    }

    // --- shaped-item overlap: two 2x2 crates in a 4x4 grid ---
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;

        int leftover1 = InventorySystem::addItem(inv, db, "crate", 1);
        int leftover2 = InventorySystem::addItem(inv, db, "crate", 1);
        assert(leftover1 == 0 && leftover2 == 0);
        assert(inv.stacks.size() == 2);
        assert(inv.stacks[0].gridX == 0 && inv.stacks[0].gridY == 0);
        // Second crate must skip the first one's 2x2 footprint entirely
        // — lands at (2, 0), not overlapping (0,0)-(1,1).
        assert(inv.stacks[1].gridX == 2 && inv.stacks[1].gridY == 0);
        std::printf("[ok] shaped overlap: second 2x2 crate correctly skips the first\n");

        // A 4x4 grid exactly fits four 2x2 crates, one per quadrant —
        // (0,0), (2,0), (0,2), (2,2). Two more must succeed here...
        int leftover3 = InventorySystem::addItem(inv, db, "crate", 1);
        int leftover4 = InventorySystem::addItem(inv, db, "crate", 1);
        assert(leftover3 == 0 && leftover4 == 0);
        assert(inv.stacks.size() == 4);
        assert(inv.stacks[2].gridX == 0 && inv.stacks[2].gridY == 2);
        assert(inv.stacks[3].gridX == 2 && inv.stacks[3].gridY == 2);
        std::printf("[ok] grid packing: all four quadrants of a 4x4 grid filled by 2x2 crates\n");

        // ...and only a fifth, with nowhere left at all, must fail.
        int leftover5 = InventorySystem::addItem(inv, db, "crate", 1);
        assert(leftover5 == 1); // nothing fit
        assert(inv.stacks.size() == 4); // no partial/invalid stack got added
        std::printf("[ok] grid-full: fifth crate correctly doesn't fit, leftover=1\n");
    }

    // --- weight limit: budget for exactly 2 of 5 arrows ---
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1.0f; // arrow weighs 0.1 -> budget for exactly 10... use a tighter budget below

        inv.maxWeight = 0.2f; // budget for exactly 2 arrows (2 * 0.1 = 0.2)
        int leftover = InventorySystem::addItem(inv, db, "arrow", 5);
        assert(leftover == 3);
        assert(inv.stacks.size() == 1);
        assert(inv.stacks[0].quantity == 2);
        std::printf("[ok] weight limit: 5 arrows into a 0.2kg budget -> 2 fit, leftover=3\n");
    }

    // --- unknown item id: nothing added, full quantity returned ---
    {
        Inventory inv;
        int leftover = InventorySystem::addItem(inv, db, "does_not_exist", 5);
        assert(leftover == 5);
        assert(inv.stacks.empty());
        std::printf("[ok] unknown item id: correctly rejected, leftover == quantity\n");
    }

    // --- moveAllTo: partial transfer, weight budget exhausted mid-move ---
    {
        Inventory source;
        source.gridWidth = 4;
        source.gridHeight = 4;
        source.maxWeight = 1000.0f;
        int leftover = InventorySystem::addItem(source, db, "arrow", 12); // -> stacks of 5, 5, 2 (see above)
        assert(leftover == 0);

        Inventory dest;
        dest.gridWidth = 4;
        dest.gridHeight = 4;
        dest.maxWeight = 0.7f; // budget for exactly 7 arrows (7 * 0.1 = 0.7)

        InventorySystem::moveAllTo(source, dest, db);

        float destWeight = InventorySystem::currentWeight(dest, db);
        float sourceWeight = InventorySystem::currentWeight(source, db);
        assert(destWeight <= dest.maxWeight + 1e-4f);

        int destTotal = 0;
        for (const auto& s : dest.stacks) destTotal += s.quantity;
        int sourceTotal = 0;
        for (const auto& s : source.stacks) sourceTotal += s.quantity;

        assert(destTotal == 7);   // dest filled to exactly its weight budget
        assert(sourceTotal == 5); // the rest (12 - 7) stayed behind in source
        assert(destTotal + sourceTotal == 12); // total quantity conserved across the move
        std::printf("[ok] moveAllTo: 12 arrows, dest budget for 7 -> dest=%d source=%d (conserved)\n",
                    destTotal, sourceTotal);
        (void)sourceWeight;
    }

    // --- stackIndexAt / removeStackAt: the shared "what's at this cell" query ---
    {
        Inventory inv;
        inv.gridWidth = 4;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        InventorySystem::addItem(inv, db, "crate", 1); // 2x2 at (0,0)

        assert(!InventorySystem::stackIndexAt(inv, db, 3, 3).has_value()); // empty cell
        auto idx = InventorySystem::stackIndexAt(inv, db, 1, 1); // bottom-right cell of the crate's footprint
        assert(idx.has_value() && *idx == 0);
        std::printf("[ok] stackIndexAt: found via a non-origin cell, correctly empty elsewhere\n");

        auto removed = InventorySystem::removeStackAt(inv, db, 1, 1);
        assert(removed.has_value());
        assert(removed->itemId == "crate");
        assert(inv.stacks.empty());
        assert(!InventorySystem::removeStackAt(inv, db, 0, 0).has_value()); // already gone -- no-op, no crash
        std::printf("[ok] removeStackAt: removed the right stack, second call on the same cell is a safe no-op\n");
    }

    // --- quickTransferStack: full transfer ---
    {
        Inventory source;
        source.gridWidth = 4;
        source.gridHeight = 4;
        source.maxWeight = 1000.0f;
        InventorySystem::addItem(source, db, "arrow", 3); // at (0,0)

        Inventory dest;
        dest.gridWidth = 4;
        dest.gridHeight = 4;
        dest.maxWeight = 1000.0f;

        bool moved = InventorySystem::quickTransferStack(source, dest, db, 0, 0);
        assert(moved);
        assert(source.stacks.empty());
        assert(dest.stacks.size() == 1 && dest.stacks[0].quantity == 3);
        std::printf("[ok] quickTransferStack: full transfer, source emptied, dest received it\n");
    }

    // --- quickTransferStack: dest has no room at all -> source untouched ---
    {
        Inventory source;
        source.gridWidth = 4;
        source.gridHeight = 4;
        source.maxWeight = 1000.0f;
        InventorySystem::addItem(source, db, "crate", 1); // at (0,0)

        Inventory dest;
        dest.gridWidth = 4;
        dest.gridHeight = 4;
        dest.maxWeight = 1000.0f;
        InventorySystem::addItem(dest, db, "crate", 1); // fills (0,0)
        InventorySystem::addItem(dest, db, "crate", 1); // fills (2,0)
        InventorySystem::addItem(dest, db, "crate", 1); // fills (0,2)
        InventorySystem::addItem(dest, db, "crate", 1); // fills (2,2) -- dest's 4x4 grid is now completely full

        bool moved = InventorySystem::quickTransferStack(source, dest, db, 0, 0);
        assert(!moved);
        assert(source.stacks.size() == 1); // completely untouched, not partially consumed
        assert(source.stacks[0].gridX == 0 && source.stacks[0].gridY == 0);
        std::printf("[ok] quickTransferStack: dest completely full -> source left exactly as it was\n");
    }


    std::printf("ALL INVENTORYSYSTEM TESTS PASSED\n");
    return 0;
}
