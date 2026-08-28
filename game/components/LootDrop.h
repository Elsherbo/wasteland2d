#pragma once

#include <string>
#include <vector>

namespace game::components {

// Attach to any entity that should spawn a lootable corpse when its
// Health reaches zero — see CombatSystem::fireWeapon's death handling.
// An entity with Health but no LootDrop just despawns, exactly like
// every entity did through Milestone 5. This is what lets the target
// dummy demonstrate looting without CombatSystem itself hardcoding
// "the dummy drops X" — CombatSystem stays generic; what a specific
// entity drops is this component's data, set wherever that entity is
// spawned (main.cpp, for now).
struct LootDrop {
    struct Entry {
        std::string itemId;
        int quantity = 1;
    };
    std::vector<Entry> items;
};

} // namespace game::components
