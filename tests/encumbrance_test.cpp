// Standalone, no-SDL, no-Box2D test of Encumbrance's speed-multiplier
// math. Only needs nlohmann::json via ItemDatabase.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>

#include "components/Inventory.h"
#include "data/ItemDatabase.h"
#include "systems/Encumbrance.h"
#include "systems/InventorySystem.h"

using game::components::Inventory;
using game::systems::Encumbrance;

namespace {
void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([{"id": "brick", "name": "Brick", "width": 1, "height": 1, "maxStack": 20, "weight": 1.0, "category": "misc"}])";
}

bool nearlyEqual(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main() {
    const std::string dbPath = "/tmp/encumbrance_test_items.json";
    writeTestItemDatabase(dbPath);
    game::data::ItemDatabase db;
    db.loadFromFile(dbPath);

    // --- under the soft cap: no penalty at all ---
    {
        Inventory inv;
        inv.gridWidth = 10;
        inv.gridHeight = 10;
        inv.softMaxWeight = 10.0f;
        inv.maxWeight = 20.0f;
        game::systems::InventorySystem::addItem(inv, db, "brick", 5); // 5kg, well under the 10kg soft cap

        assert(nearlyEqual(Encumbrance::speedMultiplier(inv, db), 1.0f));
        std::printf("[ok] under soft cap: no speed penalty\n");
    }

    // --- exactly at the soft cap: still no penalty (boundary) ---
    {
        Inventory inv;
        inv.gridWidth = 10;
        inv.gridHeight = 10;
        inv.softMaxWeight = 10.0f;
        inv.maxWeight = 20.0f;
        game::systems::InventorySystem::addItem(inv, db, "brick", 10); // exactly 10kg

        assert(nearlyEqual(Encumbrance::speedMultiplier(inv, db), 1.0f));
        std::printf("[ok] exactly at soft cap: still no penalty\n");
    }

    // --- halfway between soft and hard cap: halfway penalty ---
    {
        Inventory inv;
        inv.gridWidth = 10;
        inv.gridHeight = 10;
        inv.softMaxWeight = 10.0f;
        inv.maxWeight = 20.0f;
        game::systems::InventorySystem::addItem(inv, db, "brick", 15); // 15kg -- halfway from 10 to 20

        float expected = 1.0f - 0.5f * (1.0f - Encumbrance::kMinSpeedMultiplier);
        assert(nearlyEqual(Encumbrance::speedMultiplier(inv, db), expected));
        std::printf("[ok] halfway between soft and hard cap: halfway penalty (%.3f)\n", static_cast<double>(expected));
    }

    // --- beyond the hard cap (e.g. after a backpack downgrade): floors at kMinSpeedMultiplier ---
    {
        Inventory inv;
        inv.gridWidth = 10;
        inv.gridHeight = 10;
        // Wide-open caps while loading up, THEN shrink them -- mirrors
        // EquipmentSystem::equip's real downgrade scenario, where
        // capacity shrinks after items are already carried.
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "brick", 15); // 15kg, freely allowed under the wide-open caps

        inv.softMaxWeight = 10.0f;
        inv.maxWeight = 12.0f; // shrunk -- now already carrying more than both caps

        assert(Encumbrance::speedMultiplier(inv, db) <= Encumbrance::kMinSpeedMultiplier + 1e-4f);
        std::printf("[ok] beyond hard cap (post-downgrade scenario): floors at minimum speed, not zero\n");
    }

    // --- degenerate config (hard cap <= soft cap): fails safe, no crash ---
    {
        Inventory inv;
        inv.gridWidth = 10;
        inv.gridHeight = 10;
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "brick", 12);

        inv.softMaxWeight = 10.0f;
        inv.maxWeight = 5.0f; // hard cap BELOW soft cap -- nonsensical, must not crash or divide by a non-positive range

        assert(nearlyEqual(Encumbrance::speedMultiplier(inv, db), Encumbrance::kMinSpeedMultiplier));
        std::printf("[ok] degenerate cap configuration (hard <= soft): fails safe to minimum speed, no crash\n");
    }

    std::printf("ALL ENCUMBRANCE TESTS PASSED\n");
    return 0;
}
