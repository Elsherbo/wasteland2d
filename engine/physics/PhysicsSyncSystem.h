#pragma once

#include "ecs/Components.h"
#include "ecs/Registry.h"
#include "physics/PhysicsWorld.h"
#include "physics/RigidBody.h"

namespace engine::physics {

// Call once per frame, right after PhysicsWorld::step(). For any entity
// with both a Transform and a RigidBody, the RigidBody's b2Body is the
// source of truth — this copies its position/angle into Transform,
// which is what SpriteRenderSystem (and everything else that reads
// Transform) actually sees. Nothing should write to Transform.x/y
// directly for a physics-driven entity anymore; drive it through
// PhysicsWorld::setLinearVelocity() etc. instead, and let this system
// be the only thing that writes Transform back.
class PhysicsSyncSystem {
public:
    static void sync(ecs::Registry& registry, const PhysicsWorld& physics) {
        for (ecs::Entity e : registry.view<ecs::Transform, RigidBody>()) {
            auto& transform = registry.get<ecs::Transform>(e);
            const auto& rb = registry.get<RigidBody>(e);
            glm::vec2 pos = physics.position(rb);
            transform.x = pos.x;
            transform.y = pos.y;
            transform.rotationDegrees = physics.angleDegrees(rb);
        }
    }
};

} // namespace engine::physics
