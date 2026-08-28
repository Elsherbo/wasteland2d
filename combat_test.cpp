// Standalone, no-SDL test of CombatSystem's hitscan/damage/death/
// cooldown logic against a real PhysicsWorld — same style as the
// existing Milestone 2/3 headless tests the README references.
#include <cassert>
#include <cstdio>

#include "ecs/Registry.h"
#include "physics/PhysicsWorld.h"
#include "components/Health.h"
#include "components/Weapon.h"
#include "systems/CombatSystem.h"

int main() {
    engine::ecs::Registry registry;
    engine::physics::PhysicsWorld physics;

    engine::ecs::Entity shooter = registry.create();
    registry.emplace<game::components::Weapon>(shooter,
        game::components::Weapon{10.0f, 25.0f, 800.0f, 0.0f}); // 0 spread for a deterministic test

    engine::ecs::Entity target = registry.create();
    registry.emplace<game::components::Health>(target, game::components::Health{60.0f, 60.0f, false});

    engine::physics::BodyParams targetParams;
    targetParams.type = engine::physics::BodyType::Static;
    targetParams.position = glm::vec2(100.0f, 0.0f);
    targetParams.width = 20.0f;
    targetParams.height = 20.0f;
    auto targetBody = physics.createBody(target, targetParams);
    // Real game entities (see main.cpp's dummy setup) always carry a
    // RigidBody component — without it, CombatSystem has no way to
    // find and destroy the physics body on death. Test the real path.
    registry.emplace<engine::physics::RigidBody>(target, targetBody);

    glm::vec2 origin(0.0f, 0.0f);
    glm::vec2 aim(1.0f, 0.0f); // straight toward the target

    // Shot 1: should fire, hit, deal 25 damage, not kill (60 -> 35).
    auto r1 = game::systems::CombatSystem::fireWeapon(registry, physics, shooter, origin, aim);
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
    auto r2 = game::systems::CombatSystem::fireWeapon(registry, physics, shooter, origin, aim);
    assert(!r2.fired);
    std::printf("[ok] shot 2 (same tick): correctly blocked by cooldown\n");

    // Advance past the cooldown, then fire twice more: 35 -> 10 -> dead (despawned).
    game::systems::CombatSystem::updateCooldowns(registry, 0.2);
    auto r3 = game::systems::CombatSystem::fireWeapon(registry, physics, shooter, origin, aim);
    assert(r3.fired && r3.hit && !r3.killedTarget);
    assert(registry.get<game::components::Health>(target).current == 10.0f);
    std::printf("[ok] shot 3: hpAfter=%.0f\n", registry.get<game::components::Health>(target).current);

    game::systems::CombatSystem::updateCooldowns(registry, 0.2);
    auto r4 = game::systems::CombatSystem::fireWeapon(registry, physics, shooter, origin, aim);
    assert(r4.fired && r4.hit && r4.killedTarget);
    assert(!registry.isAlive(target)); // despawned on death
    std::printf("[ok] shot 4: killed target, entity despawned (isAlive=%d)\n", registry.isAlive(target));

    // Firing again at the now-empty spot should still register as a
    // legal shot (weapon fires), just with nothing left to hit.
    game::systems::CombatSystem::updateCooldowns(registry, 0.2);
    auto r5 = game::systems::CombatSystem::fireWeapon(registry, physics, shooter, origin, aim);
    assert(r5.fired);
    assert(!r5.hit); // target's physics body was destroyed along with the entity — nothing left to hit
    std::printf("[ok] shot 5: fired=%d hit=%d (target body correctly gone, clean miss)\n", r5.fired, r5.hit);

    std::printf("ALL COMBATSYSTEM TESTS PASSED\n");
    return 0;
}
