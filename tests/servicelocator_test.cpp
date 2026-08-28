// Test Service Locator and Resource Manager
#include "core/ServiceLocator.h"
#include "resources/ResourceManager.h"
#include <cassert>
#include <iostream>
#include <memory>

// Mock service for testing
struct MockService {
    int value = 42;
};

// Another mock service
struct AnotherService {
    std::string name = "test";
};

int main() {
    std::cout << "Testing Service Locator and Resource Manager...\n";
    
    // ===== Service Locator Tests =====
    std::cout << "\n--- Service Locator Tests ---\n";
    
    // Test 1: Provide and get service
    MockService mockService;
    engine::ServiceLocator::provide<MockService>(&mockService);
    
    auto* retrieved = engine::ServiceLocator::get<MockService>();
    assert(retrieved != nullptr && "Should retrieve service");
    assert(retrieved->value == 42 && "Service value should be correct");
    std::cout << "[ok] Provide and get service works\n";
    
    // Test 2: Multiple services
    AnotherService anotherService;
    engine::ServiceLocator::provide<AnotherService>(&anotherService);
    
    auto* another = engine::ServiceLocator::get<AnotherService>();
    assert(another != nullptr && "Should retrieve another service");
    assert(another->name == "test" && "Service name should be correct");
    std::cout << "[ok] Multiple services work\n";
    
    // Test 3: Service availability check
    assert(engine::ServiceLocator::isProvided<MockService>() && "MockService should be provided");
    assert(engine::ServiceLocator::isProvided<AnotherService>() && "AnotherService should be provided");
    std::cout << "[ok] Service availability check works\n";
    
    // Test 4: Get null for non-existent service
    struct NonExistentService {};
    auto* nullService = engine::ServiceLocator::getOrNull<NonExistentService>();
    assert(nullService == nullptr && "Should return nullptr for non-existent service");
    std::cout << "[ok] Get null for non-existent service works\n";
    
    // Test 5: Clear services
    engine::ServiceLocator::clear();
    assert(!engine::ServiceLocator::isProvided<MockService>() && "Service should be cleared");
    std::cout << "[ok] Clear services works\n";
    
    // ===== Resource Manager Tests =====
    std::cout << "\n--- Resource Manager Tests ---\n";
    
    // Note: We can't fully test TextureLoader without an SDL renderer
    // But we can test the ResourceManager structure
    
    auto resourceManager = std::make_unique<engine::ResourceManager>();
    
    // Test 6: Resource manager creation
    assert(resourceManager != nullptr && "ResourceManager should be created");
    assert(resourceManager->size() == 0 && "Cache should be empty initially");
    std::cout << "[ok] ResourceManager creation works\n";
    
    // Test 7: Cache management
    resourceManager->clear();
    assert(resourceManager->size() == 0 && "Cache should be empty after clear");
    std::cout << "[ok] Resource cache management works\n";
    
    // Test 8: Store and retrieve
    struct TestResource : public engine::Resource {
        int value = 100;
    };
    
    auto testResource = std::make_shared<TestResource>();
    resourceManager->store("test", testResource);
    
    assert(resourceManager->has("test") && "Resource should be stored");
    auto retrievedRes = resourceManager->get<TestResource>("test");
    assert(retrievedRes != nullptr && "Should retrieve resource");
    assert(retrievedRes->value == 100 && "Resource value should be correct");
    std::cout << "[ok] Store and retrieve resources works\n";
    
    // Test 9: Remove resource
    resourceManager->remove("test");
    assert(!resourceManager->has("test") && "Resource should be removed");
    std::cout << "[ok] Remove resource works\n";
    
    std::cout << "\nALL SERVICE LOCATOR AND RESOURCE MANAGER TESTS PASSED\n";
    std::cout << "(Note: Full ResourceManager tests require SDL renderer context)\n";
    
    return 0;
}
