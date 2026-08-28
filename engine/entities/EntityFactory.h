#pragma once

#include "ecs/Registry.h"
#include <glm/vec2.hpp>
#include <string>

namespace engine::entities {

// Builder pattern for constructing entities with components
class EntityBuilder {
public:
    explicit EntityBuilder(ecs::Registry& registry);
    
    // Build and return the entity
    ecs::Entity build();
    
    // Component builders (chaining)
    EntityBuilder& withPosition(glm::vec2 position);
    EntityBuilder& withVelocity(glm::vec2 velocity);
    EntityBuilder& withRotation(float rotation);
    EntityBuilder& withScale(glm::vec2 scale);
    EntityBuilder& withName(const std::string& name);
    EntityBuilder& withTag(const std::string& tag);
    
    // Physics components
    EntityBuilder& withPhysicsBody(float mass, bool isStatic = false);
    EntityBuilder& withCircleCollider(float radius);
    EntityBuilder& withBoxCollider(glm::vec2 size);
    
    // Render components
    EntityBuilder& withSprite(const std::string& texturePath);
    EntityBuilder& withColor(float r, float g, float b, float a = 1.0f);
    
    // Gameplay components
    EntityBuilder& withHealth(float maxHealth);
    EntityBuilder& withDamage(float damage);
    EntityBuilder& withLifetime(float seconds);

private:
    ecs::Registry& registry_;
    ecs::Entity entity_;
};

// Factory for creating pre-configured entities
class EntityFactory {
public:
    explicit EntityFactory(ecs::Registry& registry);
    
    // Create basic entities
    ecs::Entity createEmpty();
    ecs::Entity createFromBuilder(const EntityBuilder& builder);
    
    // Create common game entities
    ecs::Entity createPlayer(glm::vec2 position);
    ecs::Entity createEnemy(glm::vec2 position, const std::string& type);
    ecs::Entity createItem(glm::vec2 position, const std::string& itemId);
    ecs::Entity createProp(glm::vec2 position, const std::string& propType);
    ecs::Entity createProjectile(glm::vec2 position, glm::vec2 direction, float speed);
    ecs::Entity createParticle(glm::vec2 position, glm::vec2 velocity, float lifetime);
    
    // Utility
    void destroyEntity(ecs::Entity entity);
    void cloneEntity(ecs::Entity source, ecs::Entity target);

private:
    ecs::Registry& registry_;
};

} // namespace engine::entities
