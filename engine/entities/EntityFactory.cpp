#include "EntityFactory.h"
#include "core/Logger.h"

namespace engine::entities {

// EntityBuilder implementation
EntityBuilder::EntityBuilder(ecs::Registry& registry)
    : registry_(registry), entity_(registry.create()) {
}

ecs::Entity EntityBuilder::build() {
    return entity_;
}

EntityBuilder& EntityBuilder::withPosition(glm::vec2 position) {
    // In a full implementation, this would add a Transform component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting position");
    (void)position; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withVelocity(glm::vec2 velocity) {
    // In a full implementation, this would add a Velocity component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting velocity");
    (void)velocity; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withRotation(float rotation) {
    // In a full implementation, this would add rotation to Transform
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting rotation");
    (void)rotation; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withScale(glm::vec2 scale) {
    // In a full implementation, this would add scale to Transform
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting scale");
    (void)scale; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withName(const std::string& name) {
    // In a full implementation, this would add a Name component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting name:", name.c_str());
    (void)name; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withTag(const std::string& tag) {
    // In a full implementation, this would add a Tag component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting tag:", tag.c_str());
    (void)tag; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withPhysicsBody(float mass, bool isStatic) {
    // In a full implementation, this would add a PhysicsBody component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding physics body");
    (void)mass; // Suppress unused warning
    (void)isStatic; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withCircleCollider(float radius) {
    // In a full implementation, this would add a CircleCollider component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding circle collider");
    (void)radius; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withBoxCollider(glm::vec2 size) {
    // In a full implementation, this would add a BoxCollider component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding box collider");
    (void)size; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withSprite(const std::string& texturePath) {
    // In a full implementation, this would add a Sprite component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding sprite:", texturePath.c_str());
    (void)texturePath; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withColor(float r, float g, float b, float a) {
    // In a full implementation, this would add a Color component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Setting color");
    (void)r; // Suppress unused warning
    (void)g; // Suppress unused warning
    (void)b; // Suppress unused warning
    (void)a; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withHealth(float maxHealth) {
    // In a full implementation, this would add a Health component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding health");
    (void)maxHealth; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withDamage(float damage) {
    // In a full implementation, this would add a Damage component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding damage");
    (void)damage; // Suppress unused warning
    return *this;
}

EntityBuilder& EntityBuilder::withLifetime(float seconds) {
    // In a full implementation, this would add a Lifetime component
    LOG_INFO(LogCategory::ECS, "EntityBuilder: Adding lifetime");
    (void)seconds; // Suppress unused warning
    return *this;
}

// EntityFactory implementation
EntityFactory::EntityFactory(ecs::Registry& registry)
    : registry_(registry) {
}

ecs::Entity EntityFactory::createEmpty() {
    return registry_.create();
}

ecs::Entity EntityFactory::createFromBuilder(const EntityBuilder& builder) {
    return const_cast<EntityBuilder&>(builder).build();
}

ecs::Entity EntityFactory::createPlayer(glm::vec2 position) {
    LOG_INFO(LogCategory::ECS, "Creating player entity");
    return EntityBuilder(registry_)
        .withPosition(position)
        .withTag("Player")
        .withHealth(100.0f)
        .build();
}

ecs::Entity EntityFactory::createEnemy(glm::vec2 position, const std::string& type) {
    LOG_INFO(LogCategory::ECS, "Creating enemy entity:", type.c_str());
    return EntityBuilder(registry_)
        .withPosition(position)
        .withTag("Enemy")
        .withName(type)
        .withHealth(50.0f)
        .build();
}

ecs::Entity EntityFactory::createItem(glm::vec2 position, const std::string& itemId) {
    LOG_INFO(LogCategory::ECS, "Creating item entity:", itemId.c_str());
    return EntityBuilder(registry_)
        .withPosition(position)
        .withTag("Item")
        .withName(itemId)
        .build();
}

ecs::Entity EntityFactory::createProp(glm::vec2 position, const std::string& propType) {
    LOG_INFO(LogCategory::ECS, "Creating prop entity:", propType.c_str());
    return EntityBuilder(registry_)
        .withPosition(position)
        .withTag("Prop")
        .withName(propType)
        .withPhysicsBody(0.0f, true) // Static
        .build();
}

ecs::Entity EntityFactory::createProjectile(glm::vec2 position, glm::vec2 direction, float speed) {
    LOG_INFO(LogCategory::ECS, "Creating projectile entity");
    return EntityBuilder(registry_)
        .withPosition(position)
        .withVelocity(direction * speed)
        .withTag("Projectile")
        .withDamage(25.0f)
        .withLifetime(2.0f)
        .build();
}

ecs::Entity EntityFactory::createParticle(glm::vec2 position, glm::vec2 velocity, float lifetime) {
    LOG_INFO(LogCategory::ECS, "Creating particle entity");
    return EntityBuilder(registry_)
        .withPosition(position)
        .withVelocity(velocity)
        .withTag("Particle")
        .withLifetime(lifetime)
        .build();
}

void EntityFactory::destroyEntity(ecs::Entity entity) {
    LOG_INFO(LogCategory::ECS, "Destroying entity");
    registry_.destroy(entity);
}

void EntityFactory::cloneEntity(ecs::Entity source, ecs::Entity target) {
    LOG_INFO(LogCategory::ECS, "Cloning entity");
    // In a full implementation, this would copy all components from source to target
    (void)source; // Suppress unused warning
    (void)target; // Suppress unused warning
}

} // namespace engine::entities
