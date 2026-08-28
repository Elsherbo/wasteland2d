// Standalone, no-SDL test of MeleeCombatSystem: needs Box2D (via
// Damage::apply's physics-body cleanup on a kill), same dependency
// profile as combat_test.cpp. Each block uses its own fresh
// registry/physics/attacker — the attacker never moves or re-aims
// within a block, so a target left alive from an earlier block would
// otherwise keep getting hit by every later swing too.
#define SDL_MAIN_HANDLED
#include <cassert>
#include <cstdio>
#include <fstream>

#include "ecs/Registry.h"
#include "physics/PhysicsWorld.h"
#include "components/Health.h"
#include "components/Lootable.h"
#include "components/LootDrop.h"
#include "components/MeleeWeapon.h"
#include "data/ItemDatabase.h"
#include "systems/MeleeCombatSystem.h"
#include "test_common.h"

namespace {
void writeTestItemDatabase(const std::string& path) {
    std::ofstream f(path);
    f << R"([{"id": "bandage", "name": "Bandage", "width": 1, "height": 1, "maxStack": 5, "weight": 0.1, "category": "medical"}])";
}

engine::ecs::Entity makeAttacker(engine::ecs::Registry& registry) {
    engine::ecs::Entity e = registry.create();
    registry.emplace<engine::ecs::Transform>(e, 0.0, 0.0);
    registry.emplace<game::components::MeleeWeapon>(
        e, game::components::MeleeWeapon{30.0f, 50.0f, 90.0f, 4.0f, 0.0f}); // damage, range, arc, attacks/sec, cooldown
    return e;
}

engine::ecs::Entity makeTarget(engine::ecs::Registry& registry, engine::physics::PhysicsWorld& physics, float x,
                                float y, float hp = 100.0f) {
    engine::ecs::Entity e = registry.create();
    registry.emplace<engine::ecs::Transform>(e, static_cast<double>(x), static_cast<double>(y));
    registry.emplace<game::components::Health>(e, game::components::Health{hp, hp, false});
    engine::physics::BodyParams params;
    params.type = engine::physics::BodyType::Static;
    params.position = glm::vec2(x, y);
    params.width = 10.0f;
    params.height = 10.0f;
    auto body = physics.createBody(e, params);
    registry.emplace<engine::physics::RigidBody>(e, body);
    return e;
}
} // namespace

int main() {
    const std::string dbPath = getTestTempPath("melee_test_items.json");
    writeTestItemDatabase(dbPath);
    game::data::ItemDatabase itemDb;
    itemDb.loadFromFile(dbPath);

    glm::vec2 origin(0.0f, 0.0f);
    glm::vec2 aimRight(1.0f, 0.0f); // aiming along +X

    // --- directly in front, in range -> hit ---
    {
        engine::ecs::Registry registry;
        engine::physics::PhysicsWorld physics;
        engine::ecs::Entity attacker = makeAttacker(registry);
        engine::ecs::Entity target = makeTarget(registry, physics, 30.0f, 0.0f);

        auto result = game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, attacker, origin, aimRight);
        assert(result.fired);
        assert(result.hitEntities.size() == 1 && result.hitEntities[0] == target);
        assert(result.totalDamageDealt == 30.0f);
        std::printf("[ok] directly in front, in range: hit\n");

        // Immediate second swing, same block -> cooldown blocks it.
        auto result2 = game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, attacker, origin, aimRight);
        assert(!result2.fired);
        std::printf("[ok] immediate second swing correctly blocked by cooldown\n");
    }

    // --- behind the attacker, in range, but outside a 90-degree arc -> miss ---
    {
        engine::ecs::Registry registry;
        engine::physics::PhysicsWorld physics;
        engine::ecs::Entity attacker = makeAttacker(registry);
        makeTarget(registry, physics, -30.0f, 0.0f);

        auto result = game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, attacker, origin, aimRight);
        assert(result.fired);
        assert(result.hitEntities.empty());
        std::printf("[ok] target directly behind (outside the arc) correctly missed\n");
    }

    // --- in the arc, but past range -> miss ---
    {
        engine::ecs::Registry registry;
        engine::physics::PhysicsWorld physics;
        engine::ecs::Entity attacker = makeAttacker(registry);
        makeTarget(registry, physics, 200.0f, 0.0f);

        auto result = game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, attacker, origin, aimRight);
        assert(result.fired);
        assert(result.hitEntities.empty());
        std::printf("[ok] target beyond range (but in the arc) correctly missed\n");
    }

    // --- cleave: two targets in range and arc, both hit in one swing ---
    {
        engine::ecs::Registry registry;
        engine::physics::PhysicsWorld physics;
        engine::ecs::Entity attacker = makeAttacker(registry);
        engine::ecs::Entity a = makeTarget(registry, physics, 20.0f, 10.0f);  // ~26.6 deg off-axis, within the 45-deg half-arc
        engine::ecs::Entity b = makeTarget(registry, physics, 20.0f, -10.0f); // same, other side

        auto result = game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, attacker, origin, aimRight);
        assert(result.fired);
        assert(result.hitEntities.size() == 2);
        bool gotA = false, gotB = false;
        for (auto e : result.hitEntities) {
            if (e == a) gotA = true;
            if (e == b) gotB = true;
        }
        assert(gotA && gotB);
        assert(result.totalDamageDealt == 60.0f); // 30 each, both hit in the same swing
        std::printf("[ok] cleave: two targets both within range+arc, both hit in one swing\n");
    }

    // --- kill + corpse spawn via the shared Damage path ---
    {
        engine::ecs::Registry registry;
        engine::physics::PhysicsWorld physics;
        engine::ecs::Entity attacker = makeAttacker(registry);
        engine::ecs::Entity weak = makeTarget(registry, physics, 25.0f, 0.0f, 10.0f); // 10 HP, one hit (30 dmg) kills it
        registry.emplace<game::components::LootDrop>(weak, game::components::LootDrop{{{"bandage", 1}}});

        auto result = game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, attacker, origin, aimRight);
        assert(result.fired);
        assert(result.killedAny);
        assert(!registry.isAlive(weak));

        int lootableCount = 0;
        for (engine::ecs::Entity e : registry.view<game::components::Lootable>()) {
            (void)e;
            ++lootableCount;
        }
        assert(lootableCount == 1); // the corpse — confirms MeleeCombatSystem -> Damage -> spawnCorpse wiring works
        std::printf("[ok] kill via melee correctly spawns a corpse through the shared Damage path\n");
    }

    std::printf("ALL MELEECOMBATSYSTEM TESTS PASSED\n");
    return 0;
}
