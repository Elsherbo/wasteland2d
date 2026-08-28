#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "ecs/Entity.h"
#include "physics/AABB.h"
#include "physics/RigidBody.h"

namespace engine::physics {

// 1 meter = 32 pixels — chosen to match this project's tile size
// exactly, so tiles are clean 1m x 1m units with no awkward conversion
// factor to keep straight elsewhere. Box2D's solver is tuned for
// objects roughly 0.1-10 meters, which this keeps every game object
// comfortably inside of.
constexpr float kPixelsPerMeter = 32.0f;

inline glm::vec2 metersToPixels(glm::vec2 m) { return m * kPixelsPerMeter; }
inline glm::vec2 pixelsToMeters(glm::vec2 p) { return p / kPixelsPerMeter; }

enum class BodyType { Static, Dynamic };

struct BodyParams {
    BodyType type = BodyType::Dynamic;
    glm::vec2 position{0.0f, 0.0f}; // pixels, world space
    float angleDegrees = 0.0f;      // 0 = facing +X; increases counter-clockwise (Box2D's own convention)
    float width = 24.0f;            // pixels — single box fixture
    float height = 24.0f;           // pixels
    float density = 1.0f;
    float friction = 0.3f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool fixedRotation = false;     // true for the player — a top-down character doesn't tip over

    // false: createBody() creates the body itself but skips the solid
    // box fixture entirely — width/height/density/friction are then
    // ignored. For an entity that should be interactable (via
    // addCircleSensor) but not block movement — a lootable corpse, a
    // dropped bag — rather than a real physical obstacle. Defaults to
    // true so every existing call site is completely unaffected.
    bool solid = true;
};

// Reported in pixels/degrees. `entity` is whichever owner was passed to
// createBody() for the struck body — kNullEntity for anonymous static
// geometry (e.g. tilemap walls), which don't need identity.
struct RaycastHit {
    ecs::Entity entity = ecs::kNullEntity;
    glm::vec2 point{0.0f, 0.0f};
    glm::vec2 normal{0.0f, 0.0f};
    float fraction = 1.0f;
};

// A sensor fixture started (began=true) or stopped (began=false)
// overlapping another fixture. Only fired when at least one side of the
// contact is a sensor — ordinary solid-vs-solid collisions never
// produce these (that's not what "trigger" means here).
struct TriggerEvent {
    ecs::Entity a = ecs::kNullEntity;
    ecs::Entity b = ecs::kNullEntity;
    bool began = true;
};

// Owns the Box2D world. Every method here speaks engine units and
// engine types (glm::vec2 pixels, ecs::Entity, RigidBody) — no header
// that includes this one needs to also include <box2d/box2d.h>. Only
// PhysicsWorld.cpp does.
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // Creates a body with a single solid box fixture, tagged with
    // `owner` so raycasts and trigger events can report back which
    // entity was involved. Pass ecs::kNullEntity for anonymous static
    // geometry that doesn't need identity (e.g. tilemap walls).
    RigidBody createBody(ecs::Entity owner, const BodyParams& params);

    // Adds a second, sensor-only circle fixture to an existing body —
    // overlap-only, no physical collision response, layered on top of
    // whatever solid fixture the body already has. This is the
    // interaction/detection-range primitive (vehicle "press E" range,
    // door triggers) — see TriggerEvent / InteractionTracker.
    void addCircleSensor(RigidBody& body, float radiusPixels);

    void destroyBody(RigidBody& body);

    // One static body per rectangle — the standard way to turn a
    // TileMap's collision rectangles (see world::TileMap::colliders())
    // into real physics geometry once, at map load.
    void createStaticBodiesFromRects(const std::vector<AABB>& rectsPixels);

    // subStepCount is Box2D 3.x's replacement for the old velocity/
    // position iteration counts — 4 is Box2D's own recommended default.
    void step(double dt, int subStepCount = 4);

    // Drains every trigger enter/exit that happened during the most
    // recent step(). Deliberately not delivered as a live callback:
    // Box2D disallows creating/destroying bodies from inside its own
    // contact callbacks, and "show a prompt" / "open an inventory"
    // logic can easily do that indirectly. This queue is the safe
    // boundary — drain it after step(), handle events with an ordinary
    // system that's free to do whatever it wants.
    std::vector<TriggerEvent> drainTriggerEvents();

    std::optional<RaycastHit> raycast(glm::vec2 fromPixels, glm::vec2 toPixels) const;

    glm::vec2 position(const RigidBody& body) const;
    float angleDegrees(const RigidBody& body) const;
    glm::vec2 linearVelocity(const RigidBody& body) const;

    void setLinearVelocity(RigidBody& body, glm::vec2 pixelsPerSecond);
    void setAngularVelocityDegrees(RigidBody& body, float degreesPerSecond);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace engine::physics
