#pragma once

#include <cmath>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "ecs/Components.h"
#include "ecs/Registry.h"
#include "physics/PhysicsWorld.h"

#include "components/Health.h"
#include "components/MeleeWeapon.h"
#include "data/ItemDatabase.h"
#include "systems/Damage.h"

namespace game::systems {

struct SwingResult {
    bool fired = false; // false if the weapon was still on cooldown
    std::vector<engine::ecs::Entity> hitEntities; // everything actually damaged this swing
    float totalDamageDealt = 0.0f;
    bool killedAny = false;
};

// A real swing/arc hit-check, not a short-range hitscan — deliberately
// its own mechanic rather than CombatSystem with a tiny range, because
// the underlying question is different: hitscan asks "what's the first
// thing along this one ray", melee asks "what's within this arc-shaped
// area all at once" (a cleave — every valid target in range and angle
// gets hit in the same swing, not just the closest one). The two share
// only Damage::apply() — "hit something, maybe kill it, maybe spawn a
// corpse" is identical regardless of how something got hit; everything
// about *finding* what got hit is different and stays separate.
//
// Deliberately not a physics/Box2D query: this walks every
// Health-bearing entity in the registry directly (distance + angle
// check against each), rather than adding an area-query capability to
// PhysicsWorld. Fine at this game's current entity counts (a handful
// of Health-bearing things at once); would need a real broad-phase
// query (Box2D 3.x has b2World_OverlapAABB) if the entity count ever
// grows into the hundreds.
class MeleeCombatSystem {
public:
    // Call once per fixed update, before any swing() calls this tick —
    // same contract as CombatSystem::updateCooldowns.
    static void updateCooldowns(engine::ecs::Registry& registry, double dt) {
        for (engine::ecs::Entity e : registry.view<components::MeleeWeapon>()) {
            auto& weapon = registry.get<components::MeleeWeapon>(e);
            if (weapon.cooldown > 0.0f) {
                weapon.cooldown -= static_cast<float>(dt);
                if (weapon.cooldown < 0.0f) weapon.cooldown = 0.0f;
            }
        }
    }

    // attacker must have a MeleeWeapon component. aimDir must already
    // be normalized (or zero) — same convention as
    // CombatSystem::fireWeapon.
    static SwingResult swing(engine::ecs::Registry& registry, engine::physics::PhysicsWorld& physics,
                              const data::ItemDatabase& itemDb, engine::ecs::Entity attacker, glm::vec2 originPixels,
                              glm::vec2 aimDir) {
        SwingResult result;
        if (!registry.has<components::MeleeWeapon>(attacker)) return result;

        auto& weapon = registry.get<components::MeleeWeapon>(attacker);
        if (weapon.cooldown > 0.0f) return result; // still recovering from the last swing

        weapon.cooldown = 1.0f / weapon.attacksPerSecond;
        result.fired = true;

        if (aimDir.x == 0.0f && aimDir.y == 0.0f) return result; // nowhere to swing

        float halfArcRadians = (weapon.arcDegrees * 0.5f) * (3.14159265358979323846f / 180.0f);

        // Snapshot candidates before hitting anything — Damage::apply
        // can destroy entities (on a kill) partway through this loop,
        // and registry.view<>() iterating while entities are being
        // destroyed is exactly the kind of thing worth avoiding rather
        // than relying on it happening to work.
        std::vector<engine::ecs::Entity> candidates;
        for (engine::ecs::Entity e : registry.view<components::Health, engine::ecs::Transform>()) {
            if (e == attacker) continue;
            if (registry.get<components::Health>(e).dead) continue; // already dead — Damage::apply would no-op anyway
            candidates.push_back(e);
        }

        for (engine::ecs::Entity target : candidates) {
            if (!registry.isAlive(target)) continue; // could have been destroyed by an earlier hit this same swing

            const auto& t = registry.get<engine::ecs::Transform>(target);
            glm::vec2 toTarget(static_cast<float>(t.x) - originPixels.x, static_cast<float>(t.y) - originPixels.y);
            float distance = glm::length(toTarget);
            if (distance > weapon.range) continue;

            if (distance > 0.0001f) {
                glm::vec2 toTargetDir = toTarget / distance;
                float dot = glm::dot(aimDir, toTargetDir);
                dot = dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot); // clamp — float rounding can push dot slightly outside [-1,1]
                float angle = std::acos(dot);
                if (angle > halfArcRadians) continue; // outside the swing's arc
            }
            // distance <= 0.0001f: target is effectively on top of the
            // attacker — angle is undefined, treat as a hit rather than
            // dividing by ~zero.

            DamageResult damage = Damage::apply(registry, physics, itemDb, target, weapon.damage);
            if (damage.applied) {
                result.hitEntities.push_back(target);
                result.totalDamageDealt += weapon.damage;
                if (damage.killed) result.killedAny = true;
            }
        }

        return result;
    }
};

} // namespace game::systems
