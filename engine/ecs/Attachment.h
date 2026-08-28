#pragma once

#include "ecs/Entity.h"

namespace engine::ecs {

// Generic "this entity's Transform follows another entity's Transform"
// primitive. Deliberately entity-per-attachment rather than a bundled
// multi-sprite component: the sparse-set ECS already enforces one
// Sprite per entity, and entity-per-layer means Y-sorting, per-layer
// visibility, and per-layer texture swaps all fall out of systems that
// already exist (SpriteRenderSystem, Registry::view<>) instead of
// needing their own special-cased multi-sprite path. A layered
// character is one root entity (movement + physics body) plus N
// attached entities (body/hair/pants/weapon/...), each an ordinary
// Transform+Sprite entity.
//
// Not physics-driven — this is a render/gameplay-layer follow, applied
// by AttachmentSystem once per fixed update, after PhysicsSyncSystem
// has already written the parent's Transform for this tick. An
// attached entity should not also have a RigidBody of its own; nothing
// enforces that, but nothing needs it to (a hair sprite doesn't need
// to collide with anything).
struct Attachment {
    Entity parent = kNullEntity;

    // Offset from the parent's position, in world pixels. Rotated
    // along with the parent when followRotation is true; applied
    // as-is (parent-space-unaware) when false.
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // true: this entity's rotation is locked to the parent's (e.g. a
    // hair/clothing layer that should turn with the body).
    // false: this entity's Transform.rotationDegrees is left alone —
    // AttachmentSystem only ever writes position for it. This is what
    // lets a held weapon track the aim direction independently of
    // whatever the body is doing; whichever system owns aim (see
    // CombatSystem's aimDir in main.cpp) sets rotationDegrees on the
    // weapon entity directly, and AttachmentSystem never overwrites it.
    bool followRotation = true;
};

} // namespace engine::ecs
