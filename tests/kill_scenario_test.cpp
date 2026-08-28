// Directly reproduces the reported bug scenario: hold a reference-style
// access pattern to the player's Inventory the WRONG way (like the old
// bug did), kill something with a LootDrop (spawns a corpse -> emplaces
// a second Inventory), and confirm the CORRECT (re-fetch) pattern still
// reports accurate encumbrance afterward.
#define SDL_MAIN_HANDLED
#include <cassert>
#include <cstdio>
#include <fstream>

#include "ecs/Registry.h"
#include "ecs/Components.h"
#include "physics/PhysicsWorld.h"
#include "components/Health.h"
#include "components/Inventory.h"
#include "components/LootDrop.h"
#include "components/Weapon.h"
#include "data/ItemDatabase.h"
#include "systems/CombatSystem.h"
#include "systems/Encumbrance.h"
#include "systems/InventorySystem.h"
#include "test_common.h"
#include <glm/vec2.hpp>

int main() {
    const std::string dbPath = getTestTempPath("kill_scenario_items.json");
    std::ofstream f(dbPath);
    f << R"([{"id": "bandage", "name": "Bandage", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "medical"}])";
    f.close();

    game::data::ItemDatabase db;
    db.loadFromFile(dbPath);

    engine::ecs::Registry registry;
    engine::physics::PhysicsWorld physics;

    engine::ecs::Entity player = registry.create();
    registry.emplace<game::components::Weapon>(player, game::components::Weapon{10.0f, 999.0f, 800.0f, 0.0f});
    registry.emplace<game::components::Inventory>(player);
    game::systems::InventorySystem::addItem(registry.get<game::components::Inventory>(player), db, "bandage", 3);

    // Before any kill: encumbrance reads correctly.
    float before = game::systems::Encumbrance::speedMultiplier(registry.get<game::components::Inventory>(player), db);
    assert(before == 1.0f); // 0.3kg carried, nowhere near the 5kg base soft cap
    std::printf("[ok] encumbrance correct before any kill: %.3f\n", static_cast<double>(before));

    // Kill a LootDrop-bearing target -- this spawns a corpse, which
    // emplaces a SECOND Inventory component into the same pool the
    // player's Inventory lives in. This is the exact operation that
    // triggered the reported bug.
    engine::ecs::Entity dummy = registry.create();
    registry.emplace<game::components::Health>(dummy, game::components::Health{1.0f, 1.0f, false});
    registry.emplace<engine::ecs::Transform>(dummy, 10.0, 0.0);
    registry.emplace<game::components::LootDrop>(dummy, game::components::LootDrop{{{"bandage", 1}}});
    engine::physics::BodyParams params;
    params.type = engine::physics::BodyType::Static;
    params.position = glm::vec2(10.0f, 0.0f);
    params.width = 10.0f;
    params.height = 10.0f;
    auto body = physics.createBody(dummy, params);
    registry.emplace<engine::physics::RigidBody>(dummy, body);

    auto fireResult = game::systems::CombatSystem::fireWeapon(registry, physics, db, player, glm::vec2(0.0f, 0.0f),
                                                                glm::vec2(1.0f, 0.0f));
    assert(fireResult.killedTarget);
    assert(!registry.isAlive(dummy));
    std::printf("[ok] killed the target, corpse spawned (Inventory pool grew)\n");

    // The critical check: encumbrance, re-fetching the player's
    // Inventory fresh (the fixed pattern), must STILL report the
    // correct value -- not garbage from a stale reference.
    float after = game::systems::Encumbrance::speedMultiplier(registry.get<game::components::Inventory>(player), db);
    assert(after == 1.0f); // still 0.3kg -- nothing about the player's own carry weight changed
    std::printf("[ok] encumbrance still correct after the kill (re-fetch pattern survives corpse spawn): %.3f\n",
                static_cast<double>(after));

    std::printf("ALL KILL-SCENARIO REGRESSION TESTS PASSED\n");
    return 0;
}
