// Test SystemRegistry
#include "systems/SystemRegistry.h"
#include <cassert>
#include <iostream>

// Mock system for testing
class MockSystem : public engine::SystemBase {
public:
    MockSystem(const char* name) : name_(name), initialized_(false), updateCount_(0), renderCount_(0) {}
    
    void initialize() override {
        initialized_ = true;
        std::cout << "System '" << name_ << "' initialized\n";
    }
    
    void update(double dt) override {
        updateCount_++;
        (void)dt; // Suppress unused warning
    }
    
    void render() override {
        renderCount_++;
    }
    
    void shutdown() override {
        initialized_ = false;
        std::cout << "System '" << name_ << "' shutdown\n";
    }
    
    bool initialized() const { return initialized_; }
    int updateCount() const { return updateCount_; }
    int renderCount() const { return renderCount_; }

private:
    const char* name_;
    bool initialized_;
    int updateCount_;
    int renderCount_;
};

// Another mock system
class AnotherSystem : public engine::SystemBase {
public:
    AnotherSystem() : updateCount_(0) {}
    
    void update(double dt) override {
        updateCount_++;
        (void)dt; // Suppress unused warning
    }
    
    int updateCount() const { return updateCount_; }

private:
    int updateCount_;
};

int main() {
    std::cout << "Testing SystemRegistry...\n";
    
    engine::SystemRegistry registry;
    
    // Test 1: Register system
    registry.registerSystem<MockSystem>("TestSystem");
    assert(registry.hasSystem<MockSystem>() && "Should have MockSystem");
    std::cout << "[ok] Register system works\n";
    
    // Test 2: Get system
    auto* system = registry.getSystem<MockSystem>();
    assert(system != nullptr && "Should retrieve system");
    std::cout << "[ok] Get system works\n";
    
    // Test 3: Multiple systems
    registry.registerSystem<AnotherSystem>();
    assert(registry.hasSystem<AnotherSystem>() && "Should have AnotherSystem");
    assert(registry.systemCount() == 2 && "Should have 2 systems");
    std::cout << "[ok] Multiple systems work\n";
    
    // Test 4: Initialize all
    registry.initializeAll();
    assert(system->initialized() && "System should be initialized");
    std::cout << "[ok] Initialize all works\n";
    
    // Test 5: Update all
    registry.updateAll(0.016);
    assert(system->updateCount() == 1 && "System should have updated once");
    std::cout << "[ok] Update all works\n";
    
    // Test 6: Render all
    registry.renderAll();
    assert(system->renderCount() == 1 && "System should have rendered once");
    std::cout << "[ok] Render all works\n";
    
    // Test 7: Shutdown all
    registry.shutdownAll();
    assert(!system->initialized() && "System should be shutdown");
    std::cout << "[ok] Shutdown all works\n";
    
    // Test 8: Clear all
    registry.clear();
    assert(registry.systemCount() == 0 && "Should have 0 systems after clear");
    std::cout << "[ok] Clear all works\n";
    
    std::cout << "\nALL SYSTEM REGISTRY TESTS PASSED\n";
    return 0;
}
