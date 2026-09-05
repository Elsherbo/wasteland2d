// Test UIComponent
#include "ui/UIComponent.h"
#include "ui/UIStyle.h"
#include "core/Logger.h"
#include <cassert>

using namespace engine::ui;

int main() {
    // Initialize logger
    engine::LoggerConfig config;
    config.level = engine::LogLevel::Info;
    config.enableColors = true;
    engine::Logger::init(config);
    
    LOG_INFO(engine::LogCategory::UI, "Testing UIComponent...");
    
    // Test 1: Create component
    auto component = std::make_unique<UIComponent>();
    assert(component != nullptr && "Component should be created");
    LOG_INFO(engine::LogCategory::UI, "[ok] Create component works");
    
    // Test 2: Position and size
    component->setPosition(glm::vec2(100.0f, 200.0f));
    component->setSize(glm::vec2(50.0f, 50.0f));
    assert(component->getPosition() == glm::vec2(100.0f, 200.0f) && "Position should be set");
    assert(component->getSize() == glm::vec2(50.0f, 50.0f) && "Size should be set");
    LOG_INFO(engine::LogCategory::UI, "[ok] Position and size work");
    
    // Test 3: State management
    assert(component->getState() == UIState::Normal && "Initial state should be Normal");
    component->setState(UIState::Hover);
    assert(component->getState() == UIState::Hover && "State should be Hover");
    component->setVisible(false);
    assert(component->getState() == UIState::Hidden && "Hidden state should be set");
    component->setVisible(true);
    assert(component->isVisible() && "Should be visible");
    LOG_INFO(engine::LogCategory::UI, "[ok] State management works");
    
    // Test 4: Interactable
    assert(component->isInteractable() && "Should be interactable by default");
    component->setInteractable(false);
    assert(!component->isInteractable() && "Should not be interactable");
    LOG_INFO(engine::LogCategory::UI, "[ok] Interactable works");
    
    // Test 5: Children
    auto child1 = std::make_unique<UIComponent>();
    auto child2 = std::make_unique<UIComponent>();
    component->addChild(std::move(child1));
    component->addChild(std::move(child2));
    assert(component->getChildren().size() == 2 && "Should have 2 children");
    LOG_INFO(engine::LogCategory::UI, "[ok] Add children works");
    
    // Test 6: Parent relationship
    auto* child = component->getChildren()[0];
    assert(child->getParent() == component.get() && "Child should have parent");
    LOG_INFO(engine::LogCategory::UI, "[ok] Parent relationship works");
    
    // Test 7: Z-order
    component->setZOrder(10);
    assert(component->getZOrder() == 10 && "Z-order should be set");
    LOG_INFO(engine::LogCategory::UI, "[ok] Z-order works");
    
    // Test 8: Dirty flag
    assert(component->isDirty() && "Should be dirty after changes");
    component->clearDirty();
    assert(!component->isDirty() && "Should not be dirty after clear");
    LOG_INFO(engine::LogCategory::UI, "[ok] Dirty flag works");
    
    // Test 9: Hit testing
    component->setPosition(glm::vec2(0.0f, 0.0f));
    component->setSize(glm::vec2(100.0f, 100.0f));
    assert(component->containsPoint(glm::vec2(50.0f, 50.0f)) && "Point inside should be contained");
    assert(!component->containsPoint(glm::vec2(150.0f, 150.0f)) && "Point outside should not be contained");
    LOG_INFO(engine::LogCategory::UI, "[ok] Hit testing works");
    
    // Test 10: Transform
    component->setRotation(45.0f);
    component->setScale(glm::vec2(2.0f));
    component->setAnchor(glm::vec2(0.5f));
    assert(component->getRotation() == 45.0f && "Rotation should be set");
    assert(component->getScale() == glm::vec2(2.0f) && "Scale should be set");
    assert(component->getAnchor() == glm::vec2(0.5f) && "Anchor should be set");
    LOG_INFO(engine::LogCategory::UI, "[ok] Transform works");
    
    LOG_INFO(engine::LogCategory::UI, "ALL UICOMPONENT TESTS PASSED");
    
    engine::Logger::shutdown();
    return 0;
}
