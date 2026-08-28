#pragma once

#include <glm/vec2.hpp>

#include "ecs/Components.h"
#include "ecs/Registry.h"
#include "physics/Interactable.h"
#include "physics/PhysicsWorld.h"
#include "physics/RigidBody.h"
#include "render/Color.h"
#include "render/Sprite.h"

#include "components/Health.h"
#include "components/Inventory.h"
#include "components/Lootable.h"
#include "components/LootDrop.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"

namespace game::systems {

struct DamageResult {
    bool applied = false; // false if target had no Health component at all
    bool killed = false;  // true only on the hit that brought it from alive to dead
};

// The one place "hit something with Health, maybe kill it, maybe spawn
// a corpse" happens — originally lived only inside CombatSystem's
// hitscan path; extracted so a second combat mechanic (melee) doesn't
// duplicate this and risk quietly reintroducing the exact
// leaked-physics-body bug the corpse-spawning code was first written
// to fix (see the comment inline below — it's the same fix, now in one
// place instead of two).
class Damage {
public:
    // No-op (returns {false, false}) if target has no Health, is
    // already dead, or is kNullEntity. attacker is excluded from
    // self-damage checks by the *caller* (both CombatSystem and
    // MeleeCombatSystem already skip attacker == target before calling
    // this) — this function itself doesn't special-case an attacker,
    // since "can something damage itself" is a per-mechanic decision,
    // not a universal rule.
    static DamageResult apply(engine::ecs::Registry& registry, engine::physics::PhysicsWorld& physics,
                               const data::ItemDatabase& itemDb, engine::ecs::Entity target, float amount) {
        DamageResult result;
        if (target == engine::ecs::kNullEntity || !registry.has<components::Health>(target)) return result;

        auto& health = registry.get<components::Health>(target);
        bool wasAlive = !health.dead;
        components::applyDamage(health, amount);
        result.applied = true;

        if (wasAlive && health.dead) {
            result.killed = true;

            glm::vec2 deathPosition(0.0f, 0.0f);
            if (registry.has<engine::ecs::Transform>(target)) {
                const auto& t = registry.get<engine::ecs::Transform>(target);
                deathPosition = glm::vec2(static_cast<float>(t.x), static_cast<float>(t.y));
            }

            // Destroy the physics body *before* the ECS entity.
            // registry.destroy() below removes target's RigidBody
            // component, but does nothing to the actual b2Body it
            // wraps — without this, a killed entity leaves an
            // invisible, un-lootable, permanently-solid static body
            // behind: raycasts keep hitting it forever, and it would
            // keep blocking movement too.
            if (registry.has<engine::physics::RigidBody>(target)) {
                auto& rb = registry.get<engine::physics::RigidBody>(target);
                physics.destroyBody(rb);
            }

            // An entity with LootDrop spawns a lootable corpse in its
            // place instead of despawning outright — read (and use) it
            // before registry.destroy() below removes it along with
            // everything else target had. Anything without LootDrop
            // just despawns.
            if (registry.has<components::LootDrop>(target)) {
                spawnCorpse(registry, physics, itemDb, registry.get<components::LootDrop>(target), deathPosition);
            }

            registry.destroy(target);
        }

        return result;
    }

private:
    // A corpse: walkable (not solid — see BodyParams::solid), a small
    // fixed-size loot inventory (not the player's own grid dimensions;
    // a body isn't carried, so it isn't weight-limited the way a
    // player's backpack is), and an Interactable prompt via the same
    // Milestone 4 trigger system every other interactable already uses.
    static void spawnCorpse(engine::ecs::Registry& registry, engine::physics::PhysicsWorld& physics,
                             const data::ItemDatabase& itemDb, const components::LootDrop& loot,
                             glm::vec2 position) {
        engine::ecs::Entity corpse = registry.create();
        registry.emplace<engine::ecs::Transform>(corpse, static_cast<double>(position.x),
                                                  static_cast<double>(position.y));
        registry.emplace<engine::render::Sprite>(
            corpse, engine::render::Sprite{20.0f, 20.0f, engine::render::Color{80, 75, 70, 255}, 1, 0.0f});
        registry.emplace<components::Lootable>(corpse, components::Lootable{"corpse"});

        components::Inventory inventory;
        inventory.gridWidth = 4;
        inventory.gridHeight = 3;
        inventory.maxWeight = 1000.0f; // effectively unlimited — see comment above
        for (const auto& entry : loot.items) {
            // Leftover (if the hand-authored loot list somehow
            // overflows a 4x3 grid) is deliberately dropped rather than
            // spawned some other way or causing an error — a corpse
            // that can't hold everything it was assigned just holds as
            // much as it can.
            InventorySystem::addItem(inventory, itemDb, entry.itemId, entry.quantity);
        }
        registry.emplace<components::Inventory>(corpse, inventory);

        engine::physics::BodyParams corpseParams;
        corpseParams.type = engine::physics::BodyType::Static;
        corpseParams.position = position;
        corpseParams.solid = false; // walkable — a corpse shouldn't block movement, only be interactable
        auto corpseBody = physics.createBody(corpse, corpseParams);
        physics.addCircleSensor(corpseBody, 40.0f); // interaction range
        registry.emplace<engine::physics::RigidBody>(corpse, corpseBody);
        registry.emplace<engine::physics::Interactable>(corpse,
                                                          engine::physics::Interactable{"Press E to loot corpse"});
    }
};

} // namespace game::systems
