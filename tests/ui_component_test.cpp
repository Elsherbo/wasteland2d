// Test UIComponent
#include "ui/UIComponent.h"
#include "ui/UIStyle.h"
#include <cassert>
#include <iostream>

using namespace engine::ui;

int main() {
    std::cout << "Testing UIComponent...\n";
    
    // Test 1: Create component
    auto component = std::make_unique<UIComponent>();
    assert(component != nullptr && "Component should be created");
    std::cout << "[ok] Create component works\n";
    
    // Test 2: Position and size
    component->setPosition(glm::vec2(100.0f, 200.0f));
    component->setSize(glm::vec2(50.0f, 50.0f));
    assert(component->getPosition() == glm::vec2(100.0f, 200.0f) && "Position should be set");
    assert(component->getSize() == glm::vec2(50.0f, 50.0f) && "Size should be set");
    std::cout << "[ok] Position and size work\n";
    
    // Test 3: State management
    assert(component->getState() == UIState::Normal && "Initial state should be Normal");
    component->setState(UIState::Hover);
    assert(component->getState() == UIState::Hover && "State should be Hover");
    component->setVisible(false);
    assert(component->getState() == UIState::Hidden && "Hidden state should be set");
    component->setVisible(true);
    assert(component->isVisible() && "Should be visible");
    std::cout << "[ok] State management works\n";
    
    // Test 4: Interactable
    assert(component->isInteractable() && "Should be interactable by default");
    component->setInteractable(false);
    assert(!component->isInteractable() && "Should not be interactable");
    std::cout << "[ok] Interactable works\n";
    
    // Test 5: Children
    auto child1 = std::make_unique<UIComponent>();
    auto child2 = std::make_unique<UIComponent>();
    component->addChild(std::move(child1));
    component->addChild(std::move(child2));
    assert(component->getChildren().size() == 2 && "Should have 2 children");
    std::cout << "[ok] Add children works\n";
    
    // Test 6: Parent relationship
    auto* child = component->getChildren()[0];
    assert(child->getParent() == component.get() && "Child should have parent");
    std::cout << "[ok] Parent relationship works\n";
    
    // Test 7: Z-order
    component->setZOrder(10);
    assert(component->getZOrder() == 10 && "Z-order should be set");
    std::cout << "[ok] Z-order works\n";
    
    // Test 8: Dirty flag
    assert(component->isDirty() && "Should be dirty after changes");
    component->clearDirty();
    assert(!component->isDirty() && "Should not be dirty after clear");
    std::cout << "[ok] Dirty flag works\n";
    
    // Test 9: Hit testing
    component->setPosition(glm::vec2(0.0f, 0.0f));
    component->setSize(glm::vec2(100.0f, 100.0f));
    assert(component->containsPoint(glm::vec2(50.0f, 50.0f)) && "Point inside should be contained");
    assert(!component->containsPoint(glm::vec2(150.0f, 150.0f)) && "Point outside should not be contained");
    std::cout << "[ok] Hit testing works\n";
    
    // Test 10: Transform
    component->setRotation(45.0f);
    component->setScale(glm::vec2(2.0f));
    component->setAnchor(glm::vec2(0.5f));
    assert(component->getRotation() == 45.0f && "Rotation should be set");
    assert(component->getScale() == glm::vec2(2.0f) && "Scale should be set");
    assert(component->getAnchor() == glm::vec2(0.5f) && "Anchor should be set");
    std::cout << "[ok] Transform works\n";
    
    std::cout << "\nALL UICOMPONENT TESTS PASSED\n";
    return 0;
}
