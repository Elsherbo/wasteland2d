// Standalone, no-SDL, no-Box2D test of EquipmentSystem: Registry,
// Inventory, EquipmentSlots, Weapon, and MeleeWeapon are all plain
// data, so this only needs nlohmann::json (via ItemDatabase).
#include <cassert>
#include <cstdio>
#include <fstream>

#include "ecs/Registry.h"
#include "components/EquipmentSlots.h"
#include "components/Inventory.h"
#include "components/MeleeWeapon.h"
#include "components/Weapon.h"
#include "data/ItemDatabase.h"
#include "systems/EquipmentSystem.h"
#include "systems/InventorySystem.h"
#include "test_common.h"

using game::components::EquipmentSlots;
using game::components::Inventory;
using Slot = EquipmentSlots::Slot;
using game::systems::EquipmentSystem;

namespace {
void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([
        {"id": "pistol", "name": "Pistol", "width": 2, "height": 1, "maxStack": 1, "weight": 0.9, "category": "weapon",
         "weaponStats": {"kind": "ranged", "fireRate": 6.0, "damage": 25.0, "range": 800.0, "spreadDegrees": 2.0,
                          "adsZoomMultiplier": 1.15, "adsOffsetPixels": 50.0}},
        {"id": "smg", "name": "SMG", "width": 3, "height": 1, "maxStack": 1, "weight": 1.4, "category": "weapon",
         "weaponStats": {"kind": "ranged", "fireRate": 11.0, "damage": 16.0, "range": 500.0, "spreadDegrees": 4.5,
                          "adsZoomMultiplier": 1.3, "adsOffsetPixels": 75.0}},
        {"id": "combat_knife", "name": "Combat Knife", "width": 1, "height": 2, "maxStack": 1, "weight": 0.3, "category": "weapon",
         "weaponStats": {"kind": "melee", "meleeDamage": 30.0, "meleeRange": 55.0, "meleeArcDegrees": 90.0,
                          "meleeAttacksPerSecond": 2.5}},
        {"id": "school_backpack", "name": "School Backpack", "width": 2, "height": 2, "maxStack": 1, "weight": 0.5, "category": "gear", "carryCapacityKg": 15.0},
        {"id": "military_backpack", "name": "Military Backpack", "width": 2, "height": 2, "maxStack": 1, "weight": 1.5, "category": "gear", "carryCapacityKg": 60.0},
        {"id": "bandage", "name": "Bandage", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "medical"}
    ])";
}
} // namespace

int main() {
    const std::string dbPath = getTestTempPath("equipment_test_items.json");
    writeTestItemDatabase(dbPath);
    game::data::ItemDatabase db;
    db.loadFromFile(dbPath);

    // --- basic equip: ranged weapon into Primary ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "pistol", 1); // lands at (0,0), 2x1

        EquipmentSlots slots;
        bool equipped = game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 0, 0);
        assert(equipped);
        assert(slots.primaryItemId == "pistol");
        assert(inv.stacks.empty()); // pulled out of the grid
        std::printf("[ok] equip: pistol into Primary, removed from the grid\n");
    }

    // --- kind mismatch: ranged weapon refused for the Melee slot ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "pistol", 1);

        EquipmentSlots slots;
        bool equipped = game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Melee, 0, 0);
        assert(!equipped);
        assert(slots.meleeItemId.empty());
        assert(inv.stacks.size() == 1); // untouched
        std::printf("[ok] equip: ranged weapon correctly refused for the Melee slot\n");
    }

    // --- non-weapon item refused ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "bandage", 1);

        EquipmentSlots slots;
        bool equipped = game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 0, 0);
        assert(!equipped);
        assert(inv.stacks.size() == 1);
        std::printf("[ok] equip: non-weapon item correctly refused\n");
    }

    // --- swapping: equipping a new primary returns the old one to the grid ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "pistol", 1); // at (0,0)
        game::systems::InventorySystem::addItem(inv, db, "smg", 1);    // at (2,0) (3 wide)

        EquipmentSlots slots;
        assert(game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 0, 0)); // equip the pistol
        assert(slots.primaryItemId == "pistol");
        assert(inv.stacks.size() == 1); // just the smg left in the grid

        assert(game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 2, 0)); // equip the smg instead
        assert(slots.primaryItemId == "smg");
        assert(inv.stacks.size() == 1); // the pistol came back
        assert(inv.stacks[0].itemId == "pistol");
        std::printf("[ok] equip: swapping primary weapons returns the previous one to the grid\n");
    }

    // --- equip refused if the previously-equipped weapon has nowhere to go ---
    {
        Inventory inv;
        inv.gridWidth = 3;
        inv.gridHeight = 1; // 3 cells total
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "bandage", 1); // 1x1, at (0,0)
        game::systems::InventorySystem::addItem(inv, db, "pistol", 1);  // 2x1, at (1,0)-(2,0) -- grid now completely full

        EquipmentSlots slots;
        slots.primaryItemId = "smg"; // pretend an smg is already equipped (3-wide -- exactly this grid's total size)

        bool equipped = game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 1, 0); // try equipping the pistol instead
        assert(!equipped); // removing the pistol frees only 2 cells (the bandage still occupies 1) -- not enough for a 3-wide smg
        assert(slots.primaryItemId == "smg"); // unchanged
        assert(inv.stacks.size() == 2); // pistol correctly put back — nothing lost, nothing duplicated
        bool foundPistol = false;
        for (const auto& s : inv.stacks) {
            if (s.itemId == "pistol") {
                foundPistol = true;
                assert(s.gridX == 1 && s.gridY == 0); // back at its exact original position
            }
        }
        assert(foundPistol);
        std::printf("[ok] equip: refused when the previously-equipped weapon has nowhere to go -- nothing lost\n");
    }

    // --- unequip: item returns to the grid ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "pistol", 1);

        EquipmentSlots slots;
        game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 0, 0);
        assert(inv.stacks.empty());

        bool unequipped = game::systems::EquipmentSystem::unequip(inv, slots, db, Slot::Primary);
        assert(unequipped);
        assert(slots.primaryItemId.empty());
        assert(inv.stacks.size() == 1 && inv.stacks[0].itemId == "pistol");
        std::printf("[ok] unequip: pistol returned to the grid, slot cleared\n");

        assert(!game::systems::EquipmentSystem::unequip(inv, slots, db, Slot::Secondary)); // already empty -- no-op
        std::printf("[ok] unequip: empty slot correctly refused\n");
    }

    // --- unequip refused when the grid has no room ---
    {
        Inventory inv;
        inv.gridWidth = 2;
        inv.gridHeight = 1; // exactly fits one 2x1 pistol, nothing else
        inv.maxWeight = 1000.0f;
        game::systems::InventorySystem::addItem(inv, db, "pistol", 1);

        EquipmentSlots slots;
        game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Primary, 0, 0);
        assert(inv.stacks.empty());

        game::systems::InventorySystem::addItem(inv, db, "bandage", 1); // fills (0,0)
        game::systems::InventorySystem::addItem(inv, db, "bandage", 1); // fills (1,0) -- grid now completely full

        bool unequipped = game::systems::EquipmentSystem::unequip(inv, slots, db, Slot::Primary);
        assert(!unequipped); // no room
        assert(slots.primaryItemId == "pistol"); // stays equipped
        std::printf("[ok] unequip: correctly refused when the grid has no room, nothing lost\n");
    }

    // --- syncActiveWeapon: ranged equipped + active -> Weapon present, MeleeWeapon absent ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity player = registry.create();

        EquipmentSlots slots;
        slots.primaryItemId = "pistol";
        slots.activeSlot = Slot::Primary;

        game::systems::EquipmentSystem::syncActiveWeapon(registry, player, slots, db);
        assert(registry.has<game::components::Weapon>(player));
        assert(!registry.has<game::components::MeleeWeapon>(player));
        const auto& weapon = registry.get<game::components::Weapon>(player);
        assert(weapon.damage == 25.0f && weapon.fireRate == 6.0f);
        std::printf("[ok] syncActiveWeapon: primary active -> Weapon populated with pistol stats, no MeleeWeapon\n");

        slots.activeSlot = Slot::Melee; // nothing equipped there yet
        game::systems::EquipmentSystem::syncActiveWeapon(registry, player, slots, db);
        assert(!registry.has<game::components::Weapon>(player));
        assert(!registry.has<game::components::MeleeWeapon>(player));
        std::printf("[ok] syncActiveWeapon: switched to an empty melee slot -> neither Weapon nor MeleeWeapon present\n");

        slots.meleeItemId = "combat_knife";
        game::systems::EquipmentSystem::syncActiveWeapon(registry, player, slots, db);
        assert(!registry.has<game::components::Weapon>(player));
        assert(registry.has<game::components::MeleeWeapon>(player));
        const auto& melee = registry.get<game::components::MeleeWeapon>(player);
        assert(melee.damage == 30.0f && melee.arcDegrees == 90.0f);
        std::printf("[ok] syncActiveWeapon: melee active with a knife equipped -> MeleeWeapon populated, no Weapon\n");
    }

    // --- backpack: equipping sets soft/hard weight caps ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        // Base (no backpack): soft=5, hard=7.5 (Inventory.h's defaults).
        assert(inv.softMaxWeight == EquipmentSystem::kBaseCarryCapacityKg);
        game::systems::InventorySystem::addItem(inv, db, "school_backpack", 1); // 15kg capacity

        EquipmentSlots slots;
        bool equipped = game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Backpack, 0, 0);
        assert(equipped);
        assert(slots.backpackItemId == "school_backpack");

        float expectedSoft = EquipmentSystem::kBaseCarryCapacityKg + 15.0f;
        assert(inv.softMaxWeight == expectedSoft);
        assert(inv.maxWeight == expectedSoft * EquipmentSystem::kOverloadMultiplier);
        std::printf("[ok] equip backpack: soft cap = base + capacity, hard cap = soft * overload multiplier\n");
    }

    // --- backpack: unequipping resets to base capacity ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        game::systems::InventorySystem::addItem(inv, db, "school_backpack", 1);

        EquipmentSlots slots;
        game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Backpack, 0, 0);
        assert(inv.softMaxWeight > EquipmentSystem::kBaseCarryCapacityKg);

        bool unequipped = game::systems::EquipmentSystem::unequip(inv, slots, db, Slot::Backpack);
        assert(unequipped);
        assert(inv.softMaxWeight == EquipmentSystem::kBaseCarryCapacityKg);
        assert(inv.maxWeight == EquipmentSystem::kBaseCarryCapacityKg * EquipmentSystem::kOverloadMultiplier);
        std::printf("[ok] unequip backpack: caps reset to base (pockets-only)\n");
    }

    // --- backpack: swapping to a smaller one while carrying stuff
    //     applies the new, smaller caps immediately regardless of
    //     current load ---
    {
        Inventory inv;
        inv.gridWidth = 6;
        inv.gridHeight = 4;
        game::systems::InventorySystem::addItem(inv, db, "military_backpack", 1); // lands at (0,0)

        EquipmentSlots slots;
        game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Backpack, 0, 0); // grid now empty
        game::systems::InventorySystem::addItem(inv, db, "bandage", 5);            // lands at (0,0), 1x1
        game::systems::InventorySystem::addItem(inv, db, "school_backpack", 1);    // lands at (1,0), 2x2 -- skips the bandage

        bool swapped = game::systems::EquipmentSystem::equip(inv, slots, db, Slot::Backpack, 1, 0);
        assert(swapped);
        assert(slots.backpackItemId == "school_backpack");
        float expectedSoft = EquipmentSystem::kBaseCarryCapacityKg + 15.0f; // school backpack's smaller capacity now applies
        assert(inv.softMaxWeight == expectedSoft);
        std::printf("[ok] backpack downgrade: new (smaller) caps applied immediately, regardless of current load\n");
    }

    std::printf("ALL EQUIPMENTSYSTEM TESTS PASSED\n");
    return 0;
}
