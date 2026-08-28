#pragma once

#include <box2d/id.h>

namespace engine::physics {

// b2BodyId is a plain value type in Box2D 3.x — {index, world, generation},
// analogous to our own ecs::Entity handle, not the API surface itself.
// <box2d/id.h> only defines these small ID structs, not b2World/b2Body
// functions — so including it here (to store one by value) is a much
// smaller boundary crossing than including the full <box2d/box2d.h>.
// Creating, destroying, and manipulating bodies still always goes
// through PhysicsWorld's engine-shaped API — nothing outside
// engine/physics calls a b2Body_*/b2Shape_* function directly just
// because it can see this id's layout.
struct RigidBody {
    b2BodyId id = b2_nullBodyId;
};

} // namespace engine::physics
