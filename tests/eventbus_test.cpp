// Test the EventBus system
#include "core/EventBus.h"
#include <cassert>
#include <iostream>
#include <glm/vec2.hpp>

// Simple event for testing
struct TestEvent : public engine::Event {
    int value;
    explicit TestEvent(int v) : value(v) {}
};

struct OtherEvent : public engine::Event {
    std::string message;
    explicit OtherEvent(const std::string& msg) : message(msg) {}
};

int main() {
    std::cout << "Testing EventBus system...\n";
    
    // Create event bus
    auto eventBus = std::make_shared<engine::EventBus>();
    
    // Test 1: Subscribe and publish
    bool testReceived = false;
    size_t testId = eventBus->subscribe<TestEvent>(
        [&testReceived](const TestEvent& event) {
            testReceived = true;
            std::cout << "Test event received: " << event.value << "\n";
        });
    
    TestEvent testEvent(42);
    eventBus->publish(testEvent);
    
    assert(testReceived && "Test event should be received");
    std::cout << "[ok] Subscribe and publish works\n";
    
    // Test 2: Unsubscribe
    testReceived = false;
    eventBus->unsubscribe<TestEvent>(testId);
    eventBus->publish(testEvent);
    
    assert(!testReceived && "Test event should not be received after unsubscribe");
    std::cout << "[ok] Unsubscribe works\n";
    
    // Test 3: Multiple subscribers
    int counter = 0;
    eventBus->subscribe<TestEvent>(
        [&counter](const TestEvent&) { counter++; });
    eventBus->subscribe<TestEvent>(
        [&counter](const TestEvent&) { counter++; });
    
    eventBus->publish(testEvent);
    assert(counter == 2 && "Both subscribers should receive event");
    std::cout << "[ok] Multiple subscribers work\n";
    
    // Test 4: Different event types
    bool otherReceived = false;
    eventBus->subscribe<OtherEvent>(
        [&otherReceived](const OtherEvent& event) {
            otherReceived = true;
            std::cout << "Other event received: " << event.message << "\n";
        });
    
    OtherEvent otherEvent("hello");
    eventBus->publish(otherEvent);
    
    assert(otherReceived && "Other event should be received");
    assert(!testReceived && "Test subscriber should not receive other event");
    std::cout << "[ok] Different event types are isolated\n";
    
    // Test 5: Subscriber count
    size_t count = eventBus->subscriberCount<TestEvent>();
    assert(count == 2 && "Should have 2 test subscribers");
    std::cout << "[ok] Subscriber count works\n";
    
    // Test 6: Clear
    eventBus->clear();
    count = eventBus->subscriberCount<TestEvent>();
    assert(count == 0 && "Should have 0 subscribers after clear");
    std::cout << "[ok] Clear works\n";
    
    std::cout << "\nALL EVENTBUS TESTS PASSED\n";
    return 0;
}
