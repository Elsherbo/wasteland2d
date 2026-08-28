#include "physics/PhysicsWorld.h"

#include <box2d/box2d.h>

#include <cmath>

namespace engine::physics {

namespace {

constexpr float kPi = 3.14159265358979323846f;
inline float degToRad(float deg) { return deg * kPi / 180.0f; }
inline float radToDeg(float rad) { return rad * 180.0f / kPi; }

inline ecs::Entity ownerOfBody(b2BodyId bodyId) {
    if (!b2Body_IsValid(bodyId)) return ecs::kNullEntity;
    return static_cast<ecs::Entity>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyId)));
}

// Box2D's end-touch docs warn a shape may already be destroyed by the
// time the event is read back — always check validity before chasing
// shapeId -> body -> user data, or this reads freed data.
inline ecs::Entity ownerOfShapeIfValid(b2ShapeId shapeId) {
    if (!b2Shape_IsValid(shapeId)) return ecs::kNullEntity;
    return ownerOfBody(b2Shape_GetBody(shapeId));
}

} // namespace

// Box2D 3.x has no contact-listener class to subclass — sensor overlap
// events are buffered internally during b2World_Step and read back via
// b2World_GetSensorEvents() afterward. That's a simpler model than 2.x's
// callback-based one and happens to land exactly on the "drain after
// step(), never act inside a callback" boundary this wrapper already
// wanted (see PhysicsWorld.h's drainTriggerEvents() doc comment).
struct PhysicsWorld::Impl {
    Impl() {
        b2WorldDef def = b2DefaultWorldDef();
        def.gravity = {0.0f, 0.0f}; // top-down, not a platformer
        worldId = b2CreateWorld(&def);
    }

    ~Impl() { b2DestroyWorld(worldId); }

    b2WorldId worldId;
    std::vector<TriggerEvent> pendingEvents;
};

PhysicsWorld::PhysicsWorld() : impl_(std::make_unique<Impl>()) {}
PhysicsWorld::~PhysicsWorld() = default;

RigidBody PhysicsWorld::createBody(ecs::Entity owner, const BodyParams& params) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = params.type == BodyType::Dynamic ? b2_dynamicBody : b2_staticBody;
    glm::vec2 posM = pixelsToMeters(params.position);
    bodyDef.position = {posM.x, posM.y};
    bodyDef.rotation = b2MakeRot(degToRad(params.angleDegrees));
    bodyDef.fixedRotation = params.fixedRotation;
    bodyDef.linearDamping = params.linearDamping;
    bodyDef.angularDamping = params.angularDamping;
    bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(owner));

    b2BodyId bodyId = b2CreateBody(impl_->worldId, &bodyDef);

    if (params.solid) {
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = params.density;
        shapeDef.material.friction = params.friction;
        // Sensor events are opt-in per shape in Box2D 3.x, for *every*
        // shape involved — both the sensor itself (set again in
        // addCircleSensor) and whatever it's meant to detect. Defaulting
        // this on for every solid shape we create is what makes
        // "an Interactable's sensor notices the player" work without every
        // call site needing to remember to opt in.
        shapeDef.enableSensorEvents = true;

        float hw = pixelsToMeters(glm::vec2(params.width, 0.0f)).x * 0.5f;
        float hh = pixelsToMeters(glm::vec2(0.0f, params.height)).y * 0.5f;
        b2Polygon box = b2MakeBox(hw, hh);
        b2CreatePolygonShape(bodyId, &shapeDef, &box);
    }
    // params.solid == false: body exists (so addCircleSensor has
    // something to attach to) but has no fixture of its own yet —
    // nothing to collide with, nothing to raycast against.

    return RigidBody{bodyId};
}

void PhysicsWorld::addCircleSensor(RigidBody& rb, float radiusPixels) {
    if (!b2Body_IsValid(rb.id)) return;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    shapeDef.density = 0.0f;

    b2Circle circle;
    circle.center = {0.0f, 0.0f};
    circle.radius = pixelsToMeters(glm::vec2(radiusPixels, 0.0f)).x;
    b2CreateCircleShape(rb.id, &shapeDef, &circle);
}

void PhysicsWorld::destroyBody(RigidBody& rb) {
    if (!b2Body_IsValid(rb.id)) return;
    b2DestroyBody(rb.id);
    rb.id = b2_nullBodyId;
}

void PhysicsWorld::createStaticBodiesFromRects(const std::vector<AABB>& rectsPixels) {
    for (const AABB& rect : rectsPixels) {
        BodyParams params;
        params.type = BodyType::Static;
        params.position = glm::vec2(rect.x + rect.w * 0.5, rect.y + rect.h * 0.5);
        params.width = static_cast<float>(rect.w);
        params.height = static_cast<float>(rect.h);
        createBody(ecs::kNullEntity, params);
    }
}

void PhysicsWorld::step(double dt, int subStepCount) {
    b2World_Step(impl_->worldId, static_cast<float>(dt), subStepCount);

    b2SensorEvents events = b2World_GetSensorEvents(impl_->worldId);
    for (int i = 0; i < events.beginCount; ++i) {
        const b2SensorBeginTouchEvent& e = events.beginEvents[i];
        TriggerEvent te;
        te.a = ownerOfShapeIfValid(e.sensorShapeId);
        te.b = ownerOfShapeIfValid(e.visitorShapeId);
        te.began = true;
        impl_->pendingEvents.push_back(te);
    }
    for (int i = 0; i < events.endCount; ++i) {
        const b2SensorEndTouchEvent& e = events.endEvents[i];
        TriggerEvent te;
        te.a = ownerOfShapeIfValid(e.sensorShapeId);
        te.b = ownerOfShapeIfValid(e.visitorShapeId);
        te.began = false;
        impl_->pendingEvents.push_back(te);
    }
}

std::vector<TriggerEvent> PhysicsWorld::drainTriggerEvents() {
    std::vector<TriggerEvent> result;
    result.swap(impl_->pendingEvents);
    return result;
}

std::optional<RaycastHit> PhysicsWorld::raycast(glm::vec2 fromPixels, glm::vec2 toPixels) const {
    glm::vec2 fromM = pixelsToMeters(fromPixels);
    glm::vec2 toM = pixelsToMeters(toPixels);

    b2Vec2 origin{fromM.x, fromM.y};
    b2Vec2 translation{toM.x - fromM.x, toM.y - fromM.y}; // CastRayClosest wants a vector, not an endpoint

    b2RayResult result = b2World_CastRayClosest(impl_->worldId, origin, translation, b2DefaultQueryFilter());
    if (!result.hit) return std::nullopt;

    RaycastHit hit;
    hit.entity = ownerOfShapeIfValid(result.shapeId);
    hit.point = metersToPixels(glm::vec2(result.point.x, result.point.y));
    hit.normal = glm::vec2(result.normal.x, result.normal.y);
    hit.fraction = result.fraction;
    return hit;
}

glm::vec2 PhysicsWorld::position(const RigidBody& rb) const {
    if (!b2Body_IsValid(rb.id)) return glm::vec2(0.0f);
    b2Vec2 p = b2Body_GetPosition(rb.id);
    return metersToPixels(glm::vec2(p.x, p.y));
}

float PhysicsWorld::angleDegrees(const RigidBody& rb) const {
    if (!b2Body_IsValid(rb.id)) return 0.0f;
    return radToDeg(b2Rot_GetAngle(b2Body_GetRotation(rb.id)));
}

glm::vec2 PhysicsWorld::linearVelocity(const RigidBody& rb) const {
    if (!b2Body_IsValid(rb.id)) return glm::vec2(0.0f);
    b2Vec2 v = b2Body_GetLinearVelocity(rb.id);
    return metersToPixels(glm::vec2(v.x, v.y));
}

void PhysicsWorld::setLinearVelocity(RigidBody& rb, glm::vec2 pixelsPerSecond) {
    if (!b2Body_IsValid(rb.id)) return;
    glm::vec2 mps = pixelsToMeters(pixelsPerSecond);
    b2Body_SetLinearVelocity(rb.id, b2Vec2{mps.x, mps.y});
}

void PhysicsWorld::setAngularVelocityDegrees(RigidBody& rb, float degreesPerSecond) {
    if (!b2Body_IsValid(rb.id)) return;
    b2Body_SetAngularVelocity(rb.id, degToRad(degreesPerSecond));
}

} // namespace engine::physics
