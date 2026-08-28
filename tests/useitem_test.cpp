// Standalone, no-SDL, no-Box2D test of UseItemSystem. Registry,
// Inventory, QuickSlots, and Health are all plain data, so this only
// needs nlohmann::json via ItemDatabase.
#include <cassert>
#include <cstdio>
#include <fstream>

#include "ecs/Registry.h"
#include "components/Health.h"
#include "components/Inventory.h"
#include "components/QuickSlots.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"
#include "systems/UseItemSystem.h"
#include "test_common.h"

using game::components::Health;
using game::components::Inventory;
using game::components::QuickSlots;

namespace {
void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([
        {"id": "bandage", "name": "Bandage", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "medical",
         "useEffect": {"healAmount": 25.0}},
        {"id": "crate", "name": "Crate", "width": 2, "height": 2, "maxStack": 1, "weight": 5.0, "category": "misc"}
    ])";
}
} // namespace

int main() {
    const std::string dbPath = getTestTempPath("useitem_test_items.json");
    writeTestItemDatabase(dbPath);
    game::data::ItemDatabase db;
    db.loadFromFile(dbPath);

    // --- basic use: heals, consumes one unit ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();
        registry.emplace<Health>(player, Health{50.0f, 100.0f, false});

        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "bandage", 3);

        QuickSlots quickSlots;
        quickSlots.itemIds[0] = "bandage"; // key 4

        auto result = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 0);
        assert(result.used);
        assert(result.healedAmount == 25.0f);
        assert(registry.get<Health>(player).current == 75.0f);
        assert(inv.stacks.size() == 1 && inv.stacks[0].quantity == 2); // one consumed
        std::printf("[ok] basic use: healed 25, one bandage consumed (3 -> 2)\n");
    }

    // --- healing clamps to max ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();
        registry.emplace<Health>(player, Health{90.0f, 100.0f, false});

        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "bandage", 1);

        QuickSlots quickSlots;
        quickSlots.itemIds[0] = "bandage";

        auto result = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 0);
        assert(result.used);
        assert(result.healedAmount == 10.0f); // clamped: 90 + 25 would be 115, capped at 100 -> only +10 actually applied
        assert(registry.get<Health>(player).current == 100.0f);
        std::printf("[ok] healing clamps to max: 90 + 25 -> 100 (delta reported as 10, not 25)\n");
    }

    // --- last bandage consumed -> stack removed entirely ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();
        registry.emplace<Health>(player, Health{50.0f, 100.0f, false});

        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "bandage", 1);

        QuickSlots quickSlots;
        quickSlots.itemIds[0] = "bandage";

        auto result = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 0);
        assert(result.used);
        assert(inv.stacks.empty()); // the whole stack is gone, not left at quantity 0
        std::printf("[ok] last unit consumed: stack removed entirely, not left at quantity 0\n");

        // Using again with none left -> no-op.
        auto result2 = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 0);
        assert(!result2.used);
        std::printf("[ok] using again with none left: correctly refused\n");
    }

    // --- unassigned slot: no-op ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();
        registry.emplace<Health>(player, Health{50.0f, 100.0f, false});
        Inventory inv;
        QuickSlots quickSlots; // all slots empty

        auto result = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 2);
        assert(!result.used);
        std::printf("[ok] unassigned slot: correctly refused\n");
    }

    // --- item with no UseEffect (e.g. a crate): refused ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();
        registry.emplace<Health>(player, Health{50.0f, 100.0f, false});

        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "crate", 1);

        QuickSlots quickSlots;
        quickSlots.itemIds[0] = "crate";

        auto result = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 0);
        assert(!result.used);
        assert(inv.stacks.size() == 1); // untouched
        std::printf("[ok] item with no UseEffect: correctly refused, nothing consumed\n");
    }

    // --- dead entity: refused, item not consumed ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();
        registry.emplace<Health>(player, Health{0.0f, 100.0f, true}); // dead

        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.softMaxWeight = 1000.0f;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "bandage", 1);

        QuickSlots quickSlots;
        quickSlots.itemIds[0] = "bandage";

        auto result = game::systems::UseItemSystem::useQuickSlot(registry, player, inv, quickSlots, db, 0);
        assert(!result.used);
        assert(inv.stacks.size() == 1 && inv.stacks[0].quantity == 1); // untouched -- not consumed for no effect
        std::printf("[ok] dead entity: correctly refused, bandage not wasted\n");
    }

    std::printf("ALL USEITEMSYSTEM TESTS PASSED\n");
    return 0;
}
