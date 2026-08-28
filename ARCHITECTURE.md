# Wasteland2D Architecture Design

## Philosophy

**Decoupled, Component-Based, Data-Driven Architecture**

Following industry best practices from AAA game engines (Unity, Unreal, Godot, custom engines like Overwatch, Destiny, etc.).

## Core Principles

1. **Separation of Concerns** - Each system has a single, well-defined responsibility
2. **Data-Oriented Design** - Systems operate on data, not objects
3. **Event-Driven** - Systems communicate via events, not direct calls
4. **Interface-Based** - Depend on abstractions, not implementations
5. **Composition over Inheritance** - Build entities from components
6. **Dependency Injection** - Pass dependencies explicitly
7. **Layered Architecture** - Clear separation between engine and game layers

## Architecture Layers

```
┌─────────────────────────────────────────────────────────┐
│                     Game Layer                           │
│  (Game-specific logic: gameplay, content, rules)         │
│  • Game States                                          │
│  • Game Rules                                           │
│  • Content Data                                         │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   Systems Layer                          │
│  (Game systems that operate on ECS entities)             │
│  • CombatSystem                                         │
│  • InventorySystem                                      │
│  • EquipmentSystem                                      │
│  • MovementSystem                                       │
│  • AISystem                                             │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   Engine Core Layer                      │
│  (Framework systems: input, rendering, physics, etc.)    │
│  • Renderer                                             │
│  • Input Manager                                        │
│  • Physics World                                        │
│  • Resource Manager                                     │
│  • Event Bus                                            │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   Platform Layer                         │
│  (SDL, OS, hardware abstraction)                         │
│  • SDL2                                                 │
│  • OpenGL/DirectX                                       │
│  • File System                                          │
└─────────────────────────────────────────────────────────┘
```

## Core Systems

### 1. Event System (Event Bus)

**Purpose:** Decouple systems via publish/subscribe messaging

**Design:**
```cpp
// Event System - Decoupled communication
class EventBus {
public:
    template<typename EventType>
    void subscribe(std::function<void(const EventType&)> callback);
    
    template<typename EventType>
    void publish(const EventType& event);
    
    void processEvents(); // Process queued events
};

// Example Events
struct EntityKilledEvent {
    engine::ecs::Entity victim;
    engine::ecs::Entity killer;
};

struct ItemPickedUpEvent {
    engine::ecs::Entity entity;
    std::string itemId;
    int quantity;
};
```

**Benefits:**
- Systems don't need direct references to each other
- Easy to add new listeners without modifying senders
- Enables modding and extensions
- Better testability

### 2. Resource Manager

**Purpose:** Centralized asset loading, caching, and lifecycle management

**Design:**
```cpp
class ResourceManager {
public:
    template<typename T>
    std::shared_ptr<T> load(const std::string& path);
    
    template<typename T>
    void unload(const std::string& path);
    
    void clear(); // Unload all resources
    void reload(); // Reload all resources (hot-reloading)
    
private:
    std::unordered_map<std::string, std::shared_ptr<void>> resources_;
};

// Specialized resource loaders
class TextureLoader {
public:
    std::shared_ptr<Texture> load(const std::string& path);
};

class SoundLoader {
public:
    std::shared_ptr<Sound> load(const std::string& path);
};
```

**Benefits:**
- Automatic reference counting
- Prevents duplicate loading
- Centralized asset lifecycle
- Supports hot-reloading

### 3. Scene/World System

**Purpose:** Manage game worlds, loading/unloading scenes

**Design:**
```cpp
class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void update(double dt) = 0;
    virtual void render(engine::render::Renderer& renderer) = 0;
};

class SceneManager {
public:
    void loadScene(std::unique_ptr<Scene> scene);
    void transitionTo(std::unique_ptr<Scene> scene);
    Scene* currentScene();
    
private:
    std::unique_ptr<Scene> currentScene_;
    std::unique_ptr<Scene> nextScene_; // For transitions
};

// Example Scene
class GameScene : public Scene {
public:
    void onEnter() override;
    void onExit() override;
    void update(double dt) override;
    void render(engine::render::Renderer& renderer) override;
    
private:
    engine::ecs::Registry registry_;
    engine::physics::PhysicsWorld physics_;
    // Scene-specific systems
};
```

**Benefits:**
- Clean scene transitions
- Encapsulated world state
- Easy to add menu/pause/loading scenes

### 4. Game State Machine

**Purpose:** Manage high-level game states (menu, playing, paused, etc.)

**Design:**
```cpp
enum class GameState {
    MainMenu,
    Playing,
    Paused,
    Inventory,
    Dialog,
    Loading,
    GameOver
};

class GameStateManager {
public:
    void setState(GameState state);
    GameState currentState() const;
    bool canPause() const;
    bool isPaused() const;
    
private:
    GameState currentState_;
    std::stack<GameState> stateStack_; // For nested states
};
```

**Benefits:**
- Clear state management
- Prevents invalid state transitions
- Easy to add new states

### 5. Entity Factory

**Purpose:** Centralized entity creation with consistent component setup

**Design:**
```cpp
class EntityFactory {
public:
    EntityFactory(engine::ecs::Registry& registry, 
                  game::data::ItemDatabase& itemDb);
    
    // Create entities with pre-configured components
    engine::ecs::Entity createPlayer(glm::vec2 position);
    engine::ecs::Entity createEnemy(glm::vec2 position, const std::string& type);
    engine::ecs::Entity createItem(glm::vec2 position, const std::string& itemId);
    engine::ecs::Entity createCorpse(glm::vec2 position, const Inventory& loot);
    
private:
    engine::ecs::Registry& registry_;
    game::data::ItemDatabase& itemDb_;
};

// Builder pattern for complex entities
class EntityBuilder {
public:
    EntityBuilder(engine::ecs::Registry& registry);
    
    EntityBuilder& withTransform(glm::vec2 position);
    EntityBuilder& withSprite(const std::string& texturePath);
    EntityBuilder& withPhysics(const engine::physics::BodyParams& params);
    EntityBuilder& withHealth(float maxHealth);
    EntityBuilder& withInventory();
    EntityBuilder& withWeapon(const std::string& weaponId);
    
    engine::ecs::Entity build();
};
```

**Benefits:**
- Consistent entity creation
- Reusable entity templates
- Easy to spawn enemies, items, etc.

### 6. System Registry

**Purpose:** Centralized system management and dependency injection

**Design:**
```cpp
class SystemRegistry {
public:
    template<typename SystemType, typename... Args>
    void registerSystem(Args&&... args);
    
    template<typename SystemType>
    SystemType* getSystem();
    
    void updateAll(double dt);
    void renderAll(engine::render::Renderer& renderer);
    
private:
    std::unordered_map<std::type_index, std::unique_ptr<SystemBase>> systems_;
};

// Base system interface
class SystemBase {
public:
    virtual ~SystemBase() = default;
    virtual void update(double dt) {}
    virtual void render(engine::render::Renderer& renderer) {}
};

// Example system
class CombatSystem : public SystemBase {
public:
    CombatSystem(engine::ecs::Registry& registry, 
                 engine::physics::PhysicsWorld& physics,
                 EventBus& eventBus);
    
    void update(double dt) override;
    
private:
    engine::ecs::Registry& registry_;
    engine::physics::PhysicsWorld& physics_;
    EventBus& eventBus_;
};
```

**Benefits:**
- Centralized system lifecycle
- Easy dependency injection
- Consistent update/render loop

### 7. Service Locator

**Purpose:** Provide global access to services without direct dependencies

**Design:**
```cpp
class ServiceLocator {
public:
    template<typename ServiceType>
    static void provide(ServiceType* service);
    
    template<typename ServiceType>
    static ServiceType* get();
    
    static void reset(); // For testing
    
private:
    static std::unordered_map<std::type_index, void*> services_;
};

// Usage
ServiceLocator::provide<ResourceManager>(&resourceManager);
auto* resources = ServiceLocator::get<ResourceManager>();
```

**Benefits:**
- Decouples code from service implementations
- Easy to swap implementations (for testing)
- Global access when needed

## Directory Structure

```
wasteland2d/
├── engine/
│   ├── core/
│   │   ├── Application.h/cpp
│   │   ├── Logger.h/cpp
│   │   ├── EventBus.h/cpp
│   │   ├── ServiceLocator.h/cpp
│   │   └── Time.h/cpp
│   ├── ecs/
│   │   ├── Registry.h/cpp
│   │   ├── ComponentPool.h/cpp
│   │   ├── Components.h
│   │   └── Systems/
│   ├── physics/
│   │   ├── PhysicsWorld.h/cpp
│   │   └── Components.h
│   ├── render/
│   │   ├── Renderer.h/cpp
│   │   ├── Camera.h/cpp
│   │   └── Sprite.h/cpp
│   ├── input/
│   │   ├── InputManager.h/cpp
│   │   └── Action.h
│   ├── resources/
│   │   ├── ResourceManager.h/cpp
│   │   ├── TextureCache.h/cpp
│   │   └── Loaders/
│   └── audio/
│       ├── AudioManager.h/cpp
│       └── Sound.h
├── game/
│   ├── core/
│   │   ├── Game.h/cpp           // Main game class
│   │   ├── GameConfig.h
│   │   └── GameStates.h
│   ├── scenes/
│   │   ├── Scene.h/cpp
│   │   ├── SceneManager.h/cpp
│   │   ├── GameScene.h/cpp
│   │   └── MenuScene.h/cpp
│   ├── entities/
│   │   ├── EntityFactory.h/cpp
│   │   ├── EntityBuilder.h/cpp
│   │   └── Prefabs.h/cpp
│   ├── systems/
│   │   ├── CombatSystem.h/cpp
│   │   ├── InventorySystem.h/cpp
│   │   ├── EquipmentSystem.h/cpp
│   │   ├── MovementSystem.h/cpp
│   │   └── AISystem.h/cpp
│   ├── controllers/
│   │   ├── PlayerController.h/cpp
│   │   ├── AIController.h/cpp
│   │   └── CameraController.h/cpp
│   ├── ui/
│   │   ├── UIManager.h/cpp
│   │   ├── InventoryUI.h/cpp
│   │   └── HUD.h/cpp
│   ├── data/
│   │   ├── ItemDatabase.h/cpp
│   │   └── GameData.h
│   └── main.cpp                  // Minimal entry point
└── assets/
    ├── textures/
    ├── sounds/
    └── data/
```

## Implementation Order

1. **Phase 1: Core Infrastructure**
   - Event System
   - Service Locator
   - Logger (already done)

2. **Phase 2: Resource Management**
   - ResourceManager
   - Asset loaders

3. **Phase 3: Scene System**
   - Scene base class
   - SceneManager
   - GameScene

4. **Phase 4: Entity Management**
   - EntityFactory
   - EntityBuilder
   - Prefabs

5. **Phase 5: System Organization**
   - SystemRegistry
   - Extract systems from main.cpp
   - Controller classes

6. **Phase 6: Game Class**
   - Game class as central coordinator
   - Game state management
   - Main.cpp becomes minimal

## Benefits of This Architecture

**Maintainability:**
- Clear separation of concerns
- Easy to locate and modify code
- Consistent patterns throughout

**Scalability:**
- Easy to add new systems
- Easy to add new scenes
- Easy to add new entity types

**Testability:**
- Systems can be tested in isolation
- Mock implementations via Service Locator
- No global state dependencies

**Performance:**
- Data-oriented design
- Cache-friendly component access
- Efficient event system

**Extensibility:**
- Plugin system possible
- Modding support
- Hot-reloading assets

## Notes

This architecture follows patterns used in:
- **Unity:** Component-based entities, Scene system
- **Unreal:** Game framework, Actor/Component model
- **Godot:** Scene tree, Resource system
- **Custom Engines:** Overwatch (Event-driven), Destiny (Data-oriented)

The key is gradual refactoring - don't rewrite everything at once. Start with the Event System and extract one system at a time.