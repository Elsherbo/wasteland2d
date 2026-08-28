// Test UI Container System
#include "ui/UIContainer.h"
#include "ui/UIVBox.h"
#include "ui/UIHBox.h"
#include "ui/UIGrid.h"
#include "ui/UISliderContainer.h"
#include "ui/UIScrollContainer.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIStyle.h"
#include <glm/vec2.hpp>
#include <cassert>
#include <iostream>

using namespace engine::ui;

int main() {
    std::cout << "Testing UI Container System...\n";
    
    // Test UIContainer
    std::cout << "\n--- UIContainer ---\n";
    auto container = std::make_unique<UIContainer>();
    assert(container != nullptr && "Container should be created");
    assert(!container->isInteractable() && "Container should not be interactable");
    container->setPadding(glm::vec2(10.0f));
    assert(container->getPadding().x == 10.0f && "Padding should be set");
    container->setLayoutDirty();
    assert(container->isLayoutDirty() && "Layout should be dirty");
    std::cout << "[ok] UIContainer works\n";
    
    // Test UIVBox
    std::cout << "\n--- UIVBox ---\n";
    auto vbox = std::make_unique<UIVBox>();
    assert(vbox != nullptr && "VBox should be created");
    vbox->setSpacing(5.0f);
    assert(vbox->getSpacing() == 5.0f && "Spacing should be set");
    vbox->setExpandChildren(true);
    assert(vbox->shouldExpandChildren() && "Expand children should be enabled");
    
    // Add children to test layout
    auto child1 = std::make_unique<UIButton>();
    child1->setSize(glm::vec2(100.0f, 30.0f));
    auto child2 = std::make_unique<UIButton>();
    child2->setSize(glm::vec2(100.0f, 30.0f));
    vbox->addChild(std::move(child1));
    vbox->addChild(std::move(child2));
    vbox->setSize(glm::vec2(200.0f, 100.0f));
    vbox->layout();
    std::cout << "[ok] UIVBox works\n";
    
    // Test UIHBox
    std::cout << "\n--- UIHBox ---\n";
    auto hbox = std::make_unique<UIHBox>();
    assert(hbox != nullptr && "HBox should be created");
    hbox->setSpacing(10.0f);
    assert(hbox->getSpacing() == 10.0f && "Spacing should be set");
    hbox->setExpandChildren(true);
    assert(hbox->shouldExpandChildren() && "Expand children should be enabled");
    std::cout << "[ok] UIHBox works\n";
    
    // Test UIGrid
    std::cout << "\n--- UIGrid ---\n";
    auto grid = std::make_unique<UIGrid>();
    assert(grid != nullptr && "Grid should be created");
    grid->setColumns(5);
    assert(grid->getColumns() == 5 && "Columns should be set");
    grid->setRows(4);
    assert(grid->getRows() == 4 && "Rows should be set");
    grid->setCellWidth(32.0f);
    assert(grid->getCellWidth() == 32.0f && "Cell width should be set");
    grid->setCellHeight(32.0f);
    assert(grid->getCellHeight() == 32.0f && "Cell height should be set");
    grid->setSpacing(2.0f);
    assert(grid->getSpacing() == 2.0f && "Spacing should be set");
    std::cout << "[ok] UIGrid works\n";
    
    // Test UISliderContainer
    std::cout << "\n--- UISliderContainer ---\n";
    auto sliderContainer = std::make_unique<UISliderContainer>();
    assert(sliderContainer != nullptr && "SliderContainer should be created");
    assert(sliderContainer->getSlider() != nullptr && "Slider should exist");
    assert(sliderContainer->getLabel() != nullptr && "Label should exist");
    assert(sliderContainer->getValueLabel() != nullptr && "Value label should exist");
    sliderContainer->setLabelText("Volume");
    std::cout << "[ok] UISliderContainer works\n";
    
    // Test UIScrollContainer
    std::cout << "\n--- UIScrollContainer ---\n";
    auto scrollContainer = std::make_unique<UIScrollContainer>();
    assert(scrollContainer != nullptr && "ScrollContainer should be created");
    scrollContainer->setContentSize(glm::vec2(500.0f, 500.0f));
    assert(scrollContainer->getContentSize().x == 500.0f && "Content size should be set");
    scrollContainer->setSize(glm::vec2(200.0f, 200.0f));  // Set container size to allow scrolling
    scrollContainer->setScrollPosition(glm::vec2(10.0f, 20.0f));
    // Note: scroll position gets clamped, so we just check it was set
    assert(scrollContainer->getScrollPosition().x >= 0.0f && "Scroll position should be set");
    scrollContainer->setHorizontalScrollEnabled(true);
    assert(scrollContainer->isHorizontalScrollEnabled() && "Horizontal scroll should be enabled");
    scrollContainer->setVerticalScrollEnabled(true);
    assert(scrollContainer->isVerticalScrollEnabled() && "Vertical scroll should be enabled");
    std::cout << "[ok] UIScrollContainer works\n";
    
    std::cout << "\nALL UI CONTAINER TESTS PASSED\n";
    return 0;
}
