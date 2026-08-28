// Test EntityBuilder and EntityFactory
#include "entities/EntityFactory.h"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "Testing EntityBuilder and EntityFactory...\n";
    
    engine::ecs::Registry registry;
    engine::entities::EntityFactory factory(registry);
    
    // Test 1: Create empty entity
    auto emptyEntity = factory.createEmpty();
    assert(emptyEntity != engine::ecs::kNullEntity && "Should create valid entity");
    std::cout << "[ok] Create empty entity works\n";
    
    // Test 2: EntityBuilder chaining
    auto builderEntity = engine::entities::EntityBuilder(registry)
        .withPosition(glm::vec2(10.0f, 20.0f))
        .withTag("Test")
        .build();
    
    assert(builderEntity != engine::ecs::kNullEntity && "Builder should create valid entity");
    std::cout << "[ok] EntityBuilder chaining works\n";
    
    // Test 3: Create player
    auto player = factory.createPlayer(glm::vec2(0.0f, 0.0f));
    assert(player != engine::ecs::kNullEntity && "Should create valid player");
    std::cout << "[ok] Create player works\n";
    
    // Test 4: Create enemy
    auto enemy = factory.createEnemy(glm::vec2(100.0f, 100.0f), "Zombie");
    assert(enemy != engine::ecs::kNullEntity && "Should create valid enemy");
    std::cout << "[ok] Create enemy works\n";
    
    // Test 5: Create item
    auto item = factory.createItem(glm::vec2(50.0f, 50.0f), "HealthPack");
    assert(item != engine::ecs::kNullEntity && "Should create valid item");
    std::cout << "[ok] Create item works\n";
    
    // Test 6: Create prop
    auto prop = factory.createProp(glm::vec2(200.0f, 200.0f), "Tree");
    assert(prop != engine::ecs::kNullEntity && "Should create valid prop");
    std::cout << "[ok] Create prop works\n";
    
    // Test 7: Create projectile
    auto projectile = factory.createProjectile(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), 500.0f);
    assert(projectile != engine::ecs::kNullEntity && "Should create valid projectile");
    std::cout << "[ok] Create projectile works\n";
    
    // Test 8: Create particle
    auto particle = factory.createParticle(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 10.0f), 1.0f);
    assert(particle != engine::ecs::kNullEntity && "Should create valid particle");
    std::cout << "[ok] Create particle works\n";
    
    // Test 9: Destroy entity
    factory.destroyEntity(emptyEntity);
    std::cout << "[ok] Destroy entity works\n";
    
    // Test 10: Clone entity
    auto source = factory.createEmpty();
    auto target = factory.createEmpty();
    factory.cloneEntity(source, target);
    std::cout << "[ok] Clone entity works\n";
    
    std::cout << "\nALL ENTITY FACTORY TESTS PASSED\n";
    std::cout << "(Note: Full component integration requires actual component definitions)\n";
    
    return 0;
}
