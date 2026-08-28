// Standalone, no-SDL test of CombatSystem's hitscan/damage/death/
// cooldown/corpse-spawn logic against a real PhysicsWorld — same style
// as the existing Milestone 2/3 headless tests the README references.
#include <cassert>
#include <cstdio>
#include <fstream>

#include "ecs/Registry.h"
#include "physics/PhysicsWorld.h"
#include "components/Health.h"
#include "components/Inventory.h"
#include "components/Lootable.h"
#include "components/LootDrop.h"
#include "components/Weapon.h"
#include "data/ItemDatabase.h"
#include "systems/CombatSystem.h"
#include "systems/InventorySystem.h"

namespace {
void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([
        {"id": "bandage", "name": "Bandage", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "medical"}
    ])";
}
} // namespace

int main() {
    const std::string dbPath = "/tmp/combat_test_items.json";
    writeTestItemDatabase(dbPath);
    game::data::ItemDatabase itemDb;
    itemDb.loadFromFile(dbPath);

    engine::ecs::Registry registry;
    engine::physics::PhysicsWorld physics;

    engine::ecs::Entity shooter = registry.create();
    registry.emplace<game::components::Weapon>(shooter,
        game::components::Weapon{10.0f, 25.0f, 800.0f, 0.0f}); // fireRate, damage, range, spread=0 for determinism

    // --- target without LootDrop: must behave exactly like Milestone 5
    //     (plain despawn, no corpse) — this is a regression check that
    //     adding corpse-spawning didn't change the no-LootDrop path. ---
    engine::ecs::Entity target = registry.create();
    registry.emplace<game::components::Health>(target, game::components::Health{60.0f, 60.0f, false});

    engine::physics::BodyParams targetParams;
    targetParams.type = engine::physics::BodyType::Static;
    targetParams.position = glm::vec2(100.0f, 0.0f);
    targetParams.width = 20.0f;
    targetParams.height = 20.0f;
    auto targetBody = physics.createBody(target, targetParams);
    registry.emplace<engine::physics::RigidBody>(target, targetBody);

    glm::vec2 origin(0.0f, 0.0f);
    glm::vec2 aim(1.0f, 0.0f); // straight toward the target

    // Shot 1: should fire, hit, deal 25 damage, not kill (60 -> 35).
    auto r1 = game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, shooter, origin, aim);
    assert(r1.fired);
    assert(r1.hit);
    assert(!r1.killedTarget);
    assert(r1.damageDealt == 25.0f);
    assert(registry.isAlive(target));
    assert(registry.get<game::components::Health>(target).current == 35.0f);
    std::printf("[ok] shot 1: fired=%d hit=%d damage=%.0f hpAfter=%.0f\n",
                r1.fired, r1.hit, r1.damageDealt, registry.get<game::components::Health>(target).current);

    // Immediately-repeated shot: weapon should still be on cooldown
    // (fireRate=10/sec -> 0.1s cooldown), so this should not fire.
    auto r2 = game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, shooter, origin, aim);
    assert(!r2.fired);
    std::printf("[ok] shot 2 (same tick): correctly blocked by cooldown\n");

    // Advance past the cooldown, then fire twice more: 35 -> 10 -> dead (despawned).
    game::systems::CombatSystem::updateCooldowns(registry, 0.2);
    auto r3 = game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, shooter, origin, aim);
    assert(r3.fired && r3.hit && !r3.killedTarget);
    assert(registry.get<game::components::Health>(target).current == 10.0f);
    std::printf("[ok] shot 3: hpAfter=%.0f\n", registry.get<game::components::Health>(target).current);

    game::systems::CombatSystem::updateCooldowns(registry, 0.2);
    auto r4 = game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, shooter, origin, aim);
    assert(r4.fired && r4.hit && r4.killedTarget);
    assert(!registry.isAlive(target)); // despawned on death
    std::printf("[ok] shot 4: killed target, entity despawned (isAlive=%d)\n", registry.isAlive(target));

    // No LootDrop on `target` -> no corpse should exist anywhere yet.
    int lootableCountBefore = 0;
    for (engine::ecs::Entity e : registry.view<game::components::Lootable>()) { (void)e; ++lootableCountBefore; }
    assert(lootableCountBefore == 0);
    std::printf("[ok] no LootDrop -> no corpse spawned (regression check)\n");

    // Firing again at the now-empty spot should still register as a
    // legal shot (weapon fires), just with nothing left to hit.
    game::systems::CombatSystem::updateCooldowns(registry, 0.2);
    auto r5 = game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, shooter, origin, aim);
    assert(r5.fired);
    assert(!r5.hit); // target's physics body was destroyed along with the entity — nothing left to hit
    std::printf("[ok] shot 5: fired=%d hit=%d (target body correctly gone, clean miss)\n", r5.fired, r5.hit);

    // --- target WITH LootDrop: killing it must spawn a lootable corpse ---
    engine::ecs::Entity looter_target = registry.create();
    registry.emplace<game::components::Health>(looter_target, game::components::Health{10.0f, 10.0f, false});
    registry.emplace<engine::ecs::Transform>(looter_target, 200.0, 5.0); // distinct position to verify the corpse lands here
    registry.emplace<game::components::LootDrop>(looter_target,
        game::components::LootDrop{{{"bandage", 2}}});

    engine::physics::BodyParams looterParams;
    looterParams.type = engine::physics::BodyType::Static;
    looterParams.position = glm::vec2(200.0f, 5.0f);
    looterParams.width = 20.0f;
    looterParams.height = 20.0f;
    auto looterBody = physics.createBody(looter_target, looterParams);
    registry.emplace<engine::physics::RigidBody>(looter_target, looterBody);

    glm::vec2 origin2(0.0f, 5.0f);
    glm::vec2 aim2(1.0f, 0.0f);

    game::components::Weapon& shooterWeapon = registry.get<game::components::Weapon>(shooter);
    shooterWeapon.cooldown = 0.0f; // force off cooldown regardless of prior shots' timing
    auto r6 = game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, shooter, origin2, aim2);
    assert(r6.fired && r6.hit && r6.killedTarget); // 25 damage one-shots a 10 HP target
    assert(!registry.isAlive(looter_target));
    std::printf("[ok] shot 6: killed a LootDrop-bearing target\n");

    // Exactly one corpse should now exist, with the loot transferred in.
    engine::ecs::Entity corpse = engine::ecs::kNullEntity;
    int lootableCountAfter = 0;
    for (engine::ecs::Entity e : registry.view<game::components::Lootable>()) {
        corpse = e;
        ++lootableCountAfter;
    }
    assert(lootableCountAfter == 1);
    assert(corpse != engine::ecs::kNullEntity);
    std::printf("[ok] exactly one corpse spawned\n");

    assert(registry.has<engine::ecs::Transform>(corpse));
    const auto& corpseTransform = registry.get<engine::ecs::Transform>(corpse);
    assert(corpseTransform.x == 200.0 && corpseTransform.y == 5.0); // spawned at the death position, not the shooter's
    std::printf("[ok] corpse spawned at the death position (200, 5)\n");

    assert(registry.has<game::components::Inventory>(corpse));
    const auto& corpseInventory = registry.get<game::components::Inventory>(corpse);
    assert(corpseInventory.stacks.size() == 1);
    assert(corpseInventory.stacks[0].itemId == "bandage");
    assert(corpseInventory.stacks[0].quantity == 2);
    std::printf("[ok] corpse inventory contains the 2 bandages from LootDrop\n");

    assert(registry.has<engine::physics::Interactable>(corpse));
    assert(registry.has<engine::physics::RigidBody>(corpse));
    std::printf("[ok] corpse has an Interactable prompt and a RigidBody (for the interaction-range sensor)\n");

    // Looting: transfer the corpse's inventory into a player inventory
    // via InventorySystem::moveAllTo — this is the actual mechanism
    // main.cpp's "press E to loot" interaction uses.
    game::components::Inventory playerInventory;
    playerInventory.gridWidth = 6;
    playerInventory.gridHeight = 4;
    playerInventory.maxWeight = 40.0f;
    game::systems::InventorySystem::moveAllTo(registry.get<game::components::Inventory>(corpse), playerInventory,
                                               itemDb);
    assert(playerInventory.stacks.size() == 1);
    assert(playerInventory.stacks[0].itemId == "bandage" && playerInventory.stacks[0].quantity == 2);
    assert(registry.get<game::components::Inventory>(corpse).stacks.empty()); // corpse fully looted
    std::printf("[ok] looting: corpse's 2 bandages moved into a player inventory via moveAllTo\n");

    std::printf("ALL COMBATSYSTEM TESTS PASSED\n");
    return 0;
}
