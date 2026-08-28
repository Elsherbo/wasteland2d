#pragma once

#include <cmath>
#include <random>

#include <glm/vec2.hpp>

#include "ecs/Registry.h"
#include "physics/PhysicsWorld.h"

#include "components/Weapon.h"
#include "data/ItemDatabase.h"
#include "systems/Damage.h"

namespace game::systems {

// What main.cpp needs back from a fire attempt, purely for
// presentation (muzzle flash, screen shake, hit markers) — CombatSystem
// itself already applied damage and handled death before returning.
struct FireResult {
    bool fired = false;   // false if the weapon was still on cooldown
    bool hit = false;     // true if the raycast struck something at all (wall included)
    bool killedTarget = false;
    glm::vec2 hitPoint{0.0f, 0.0f};
    engine::ecs::Entity hitEntity = engine::ecs::kNullEntity; // kNullEntity for anonymous geometry (walls)
    float damageDealt = 0.0f; // 0 unless hitEntity had a Health component

    // Where the shot actually stopped — hitPoint on a hit, or the
    // full-range point on a miss. Set whenever fired is true (even a
    // miss has an endpoint), so callers can draw a tracer without
    // recomputing range/spread math that already happened in here.
    glm::vec2 shotEnd{0.0f, 0.0f};
};

// Pure hitscan (raycast) ranged combat. Melee is a deliberately
// separate mechanic — see MeleeCombatSystem — not a variant of this
// one; the two share only Damage::apply() (see Damage.h) for the
// "hit something, maybe kill it, maybe spawn a corpse" part, which is
// identical regardless of how something got hit.
class CombatSystem {
public:
    // Call once per fixed update, before any fireWeapon() calls this
    // tick, so a weapon that was fired last tick can come off cooldown
    // in time to fire again this tick.
    static void updateCooldowns(engine::ecs::Registry& registry, double dt) {
        for (engine::ecs::Entity e : registry.view<components::Weapon>()) {
            auto& weapon = registry.get<components::Weapon>(e);
            if (weapon.cooldown > 0.0f) {
                weapon.cooldown -= static_cast<float>(dt);
                if (weapon.cooldown < 0.0f) weapon.cooldown = 0.0f;
            }
        }
    }

    // shooter must have a Weapon component. aimDir must already be
    // normalized (or zero) — callers already compute this for
    // rendering/UI purposes, so CombatSystem doesn't redo it. itemDb is
    // only consulted if this shot kills something with a LootDrop (see
    // Damage::apply) — always required rather than optional, since
    // whether that happens isn't known until the raycast resolves.
    static FireResult fireWeapon(engine::ecs::Registry& registry, engine::physics::PhysicsWorld& physics,
                                  const data::ItemDatabase& itemDb, engine::ecs::Entity shooter,
                                  glm::vec2 originPixels, glm::vec2 aimDir) {
        FireResult result;
        if (!registry.has<components::Weapon>(shooter)) return result;

        auto& weapon = registry.get<components::Weapon>(shooter);
        if (weapon.cooldown > 0.0f) return result; // still reloading/cycling

        weapon.cooldown = 1.0f / weapon.fireRate;
        result.fired = true;

        if (aimDir.x == 0.0f && aimDir.y == 0.0f) return result; // nowhere to shoot

        glm::vec2 spreadAim = applySpread(aimDir, weapon.spreadDegrees);
        glm::vec2 target = originPixels + spreadAim * weapon.range;
        result.shotEnd = target; // overwritten below if the ray actually hits something sooner

        auto hit = physics.raycast(originPixels, target);
        if (!hit.has_value()) return result;

        result.hit = true;
        result.hitPoint = hit->point;
        result.hitEntity = hit->entity;
        result.shotEnd = hit->point;

        if (hit->entity != shooter) {
            DamageResult damage = Damage::apply(registry, physics, itemDb, hit->entity, weapon.damage);
            if (damage.applied) result.damageDealt = weapon.damage;
            result.killedTarget = damage.killed;
        }

        return result;
    }

private:
    // Rotates aimDir by a random angle in [-spreadDegrees, +spreadDegrees].
    // Plain 2D rotation by hand rather than glm::rotate(), since that's
    // a GTX extension (needs GLM_ENABLE_EXPERIMENTAL) for what's a
    // three-line formula — same call PhysicsWorld.cpp already made for
    // its own degToRad/radToDeg helpers rather than pulling in an
    // extension for something this small.
    static glm::vec2 applySpread(glm::vec2 aimDir, float spreadDegrees) {
        if (spreadDegrees <= 0.0f) return aimDir;

        static thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> dist(-spreadDegrees, spreadDegrees);
        float radians = dist(rng) * (3.14159265358979323846f / 180.0f);

        float c = std::cos(radians);
        float s = std::sin(radians);
        return glm::vec2(aimDir.x * c - aimDir.y * s, aimDir.x * s + aimDir.y * c);
    }
};

} // namespace game::systems
