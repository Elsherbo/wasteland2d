#pragma once

#include "core/EventBus.h"
#include "ecs/Registry.h"
#include <glm/vec2.hpp>
#include <string>

namespace game::events {

// Combat events
struct EntityKilledEvent : public engine::Event {
    engine::ecs::Entity victim;
    engine::ecs::Entity killer;
    float damageDealt;
    
    EntityKilledEvent(engine::ecs::Entity v, engine::ecs::Entity k, float dmg)
        : victim(v), killer(k), damageDealt(dmg) {}
};

struct DamageEvent : public engine::Event {
    engine::ecs::Entity target;
    engine::ecs::Entity source;
    float damage;
    
    DamageEvent(engine::ecs::Entity t, engine::ecs::Entity s, float dmg)
        : target(t), source(s), damage(dmg) {}
};

// Inventory events
struct ItemPickedUpEvent : public engine::Event {
    engine::ecs::Entity entity;
    std::string itemId;
    int quantity;
    
    ItemPickedUpEvent(engine::ecs::Entity e, const std::string& id, int qty)
        : entity(e), itemId(id), quantity(qty) {}
};

struct ItemUsedEvent : public engine::Event {
    engine::ecs::Entity entity;
    std::string itemId;
    float effectValue;
    
    ItemUsedEvent(engine::ecs::Entity e, const std::string& id, float val)
        : entity(e), itemId(id), effectValue(val) {}
};

struct InventoryFullEvent : public engine::Event {
    engine::ecs::Entity entity;
    
    explicit InventoryFullEvent(engine::ecs::Entity e) : entity(e) {}
};

// Equipment events
struct ItemEquippedEvent : public engine::Event {
    engine::ecs::Entity entity;
    std::string itemId;
    std::string slot;
    
    ItemEquippedEvent(engine::ecs::Entity e, const std::string& id, const std::string& s)
        : entity(e), itemId(id), slot(s) {}
};

struct ItemUnequippedEvent : public engine::Event {
    engine::ecs::Entity entity;
    std::string slot;
    
    ItemUnequippedEvent(engine::ecs::Entity e, const std::string& s)
        : entity(e), slot(s) {}
};

// Physics events
struct CollisionEvent : public engine::Event {
    engine::ecs::Entity entityA;
    engine::ecs::Entity entityB;
    
    CollisionEvent(engine::ecs::Entity a, engine::ecs::Entity b)
        : entityA(a), entityB(b) {}
};

struct TriggerEnterEvent : public engine::Event {
    engine::ecs::Entity entity;
    std::string triggerName;
    
    TriggerEnterEvent(engine::ecs::Entity e, const std::string& name)
        : entity(e), triggerName(name) {}
};

// Game state events
struct GamePausedEvent : public engine::Event {
    bool paused;
    
    explicit GamePausedEvent(bool p) : paused(p) {}
};

struct LevelLoadedEvent : public engine::Event {
    std::string levelName;
    
    explicit LevelLoadedEvent(const std::string& name) : levelName(name) {}
};

// Player events
struct PlayerDeathEvent : public engine::Event {
    engine::ecs::Entity player;
    
    explicit PlayerDeathEvent(engine::ecs::Entity p) : player(p) {}
};

struct PlayerRespawnEvent : public engine::Event {
    engine::ecs::Entity player;
    glm::vec2 position;
    
    PlayerRespawnEvent(engine::ecs::Entity p, glm::vec2 pos)
        : player(p), position(pos) {}
};

} // namespace game::events
