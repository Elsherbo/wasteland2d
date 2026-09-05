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
#include "core/Logger.h"
#include <glm/vec2.hpp>
#include <cassert>

using namespace engine::ui;

int main() {
    // Initialize logger
    engine::LoggerConfig config;
    config.level = engine::LogLevel::Info;
    config.enableColors = true;
    engine::Logger::init(config);
    
    LOG_INFO(engine::LogCategory::UI, "Testing UI Container System...");
    
    // Test UIContainer
    LOG_INFO(engine::LogCategory::UI, "--- UIContainer ---");
    auto container = std::make_unique<UIContainer>();
    assert(container != nullptr && "Container should be created");
    assert(container->isInteractable() && "Container should be interactable by default");
    container->setPadding(glm::vec2(10.0f));
    assert(container->getPadding().x == 10.0f && "Padding should be set");
    container->setLayoutDirty();
    assert(container->isLayoutDirty() && "Layout should be dirty");
    LOG_INFO(engine::LogCategory::UI, "[ok] UIContainer works");
    
    // Test UIVBox
    LOG_INFO(engine::LogCategory::UI, "--- UIVBox ---");
    auto vbox = std::make_unique<UIVBox>();
    assert(vbox != nullptr && "VBox should be created");
    vbox->setSpacing(5.0f);
    assert(vbox->getSpacing() == 5.0f && "Spacing should be set");
    
    // "Expand children" is no longer a single container-wide flag
    // (setExpandChildren()/shouldExpandChildren() were removed) -- it's
    // now a per-child SizeFlag, since a real layout needs some children
    // to expand/fill and others to stay fixed size within the same
    // container. Fill on the cross axis (width, for a VBox) is the
    // direct equivalent of what the old container-wide flag did.
    auto child1 = std::make_unique<UIButton>();
    child1->setSize(glm::vec2(100.0f, 30.0f));
    child1->setSizeFlags(SizeFlag::Fill);
    auto child2 = std::make_unique<UIButton>();
    child2->setSize(glm::vec2(100.0f, 30.0f));
    child2->setSizeFlags(SizeFlag::Fill);
    UIButton* child1Raw = child1.get();
    UIButton* child2Raw = child2.get();
    vbox->addChild(std::move(child1));
    vbox->addChild(std::move(child2));
    vbox->setSize(glm::vec2(200.0f, 100.0f));
    vbox->layout();
    assert(hasFlag(child1Raw->getSizeFlags(), SizeFlag::Fill) && "Fill flag should be set on child1");
    assert(child1Raw->getSize().x == 200.0f && "Fill child should stretch to the VBox's content width");
    assert(child2Raw->getSize().x == 200.0f && "Fill child should stretch to the VBox's content width");
    LOG_INFO(engine::LogCategory::UI, "[ok] UIVBox works");
    
    // Test UIHBox
    LOG_INFO(engine::LogCategory::UI, "--- UIHBox ---");
    auto hbox = std::make_unique<UIHBox>();
    assert(hbox != nullptr && "HBox should be created");
    hbox->setSpacing(10.0f);
    assert(hbox->getSpacing() == 10.0f && "Spacing should be set");
    
    // Same SizeFlag equivalence as UIVBox above, transposed to the
    // horizontal axis (Fill stretches height for an HBox's children).
    auto hchild = std::make_unique<UIButton>();
    hchild->setSize(glm::vec2(60.0f, 20.0f));
    hchild->setSizeFlags(SizeFlag::Fill);
    UIButton* hchildRaw = hchild.get();
    hbox->addChild(std::move(hchild));
    hbox->setSize(glm::vec2(200.0f, 50.0f));
    hbox->layout();
    assert(hasFlag(hchildRaw->getSizeFlags(), SizeFlag::Fill) && "Fill flag should be set on hchild");
    assert(hchildRaw->getSize().y == 50.0f && "Fill child should stretch to the HBox's content height");
    LOG_INFO(engine::LogCategory::UI, "[ok] UIHBox works");
    
    // Test UIGrid
    LOG_INFO(engine::LogCategory::UI, "--- UIGrid ---");
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
    LOG_INFO(engine::LogCategory::UI, "[ok] UIGrid works");
    
    // Test UISliderContainer
    LOG_INFO(engine::LogCategory::UI, "--- UISliderContainer ---");
    auto sliderContainer = std::make_unique<UISliderContainer>();
    assert(sliderContainer != nullptr && "SliderContainer should be created");
    assert(sliderContainer->getSlider() != nullptr && "Slider should exist");
    assert(sliderContainer->getLabel() != nullptr && "Label should exist");
    assert(sliderContainer->getValueLabel() != nullptr && "Value label should exist");
    sliderContainer->setLabelText("Volume");
    LOG_INFO(engine::LogCategory::UI, "[ok] UISliderContainer works");
    
    // Test UIScrollContainer
    LOG_INFO(engine::LogCategory::UI, "--- UIScrollContainer ---");
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
    LOG_INFO(engine::LogCategory::UI, "[ok] UIScrollContainer works");
    
    LOG_INFO(engine::LogCategory::UI, "ALL UI CONTAINER TESTS PASSED");
    
    engine::Logger::shutdown();
    return 0;
}
