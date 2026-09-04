// HUD & Inventory Demo
//
// Deliberately composed *differently* from ui_professional_demo.cpp to
// pressure-test the UI framework on patterns the settings panel never
// touched:
//   - multiple independent top-level components on one layer (the settings
//     demo only ever had a single root component)
//   - a second UILayer ("Modal") stacked above the first, used for real
//   - UIButton toggle mode (radio-style hotbar selection)
//   - UIGrid nested inside a UIScrollContainer, with *interactive* cells
//     (buttons, not static color swatches) -- and enough of them (24) that
//     scrolling is actually required
//   - raw UIImage elements positioned absolutely (a health/mana bar),
//     outside of any Box container's layout
//   - UIHBox as a *fixed-size* row (hotbar) rather than autoSize
//
// If this compiles and behaves correctly using the exact same engine/ui
// sources as the settings demo, that's real evidence the fixes made against
// that one demo generalize -- not just curve-fit to it.
#include <SDL.h>
#include <SDL_ttf.h>
#include <glm/vec2.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "ui/UIComponent.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UIVBox.h"
#include "ui/UIHBox.h"
#include "ui/UIGrid.h"
#include "ui/UIScrollContainer.h"
#include "ui/UIRenderer.h"
#include "ui/UIStyle.h"
#include "ui/UIManager.h"
#include "ui/UIDefaultTheme.h"
#include "ui/InputEvent.h"
#include "render/Font.h"
#include "render/TextRenderer.h"
#include "render/Color.h"

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 700;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow(
        "HUD & Inventory Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        TTF_Quit(); SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return 1;
    }
    
    engine::render::Font font("C:\\Windows\\Fonts\\arial.ttf", 14);
    engine::render::Font headingFont("C:\\Windows\\Fonts\\arial.ttf", 20);
    engine::render::TextRenderer textRenderer(renderer);
    
    engine::ui::UIManager uiManager;
    auto& themeManager = uiManager.getThemeManager();
    engine::ui::UIRenderer uiRenderer(renderer, textRenderer, font, themeManager, &headingFont);
    engine::ui::installDefaultDarkTheme(themeManager, uiRenderer);
    
    // UIManager's constructor already creates "Background"/"Game"/"UI"/
    // "Overlay"/"Modal" layers at ascending zOrder -- "Modal" (40) sits
    // above "UI" (20), which is exactly the stacking a popup needs. This
    // has existed since before this demo but nothing has ever used it.
    engine::ui::UILayer* uiLayer = uiManager.getLayer("UI");
    engine::ui::UILayer* modalLayer = uiManager.getLayer("Modal");
    
    // Every top-level component added to a layer also needs to be rendered
    // explicitly -- this demo (like the settings one) renders manually
    // rather than through a layer-driven render pass, so ownership +
    // render-order both live in this vector.
    std::vector<std::unique_ptr<engine::ui::UIComponent>> uiComponents;
    std::vector<std::unique_ptr<engine::ui::UIComponent>> modalComponents;
    
    // === Modal (built first so its raw pointers can be captured by the
    // inventory slot buttons' click handlers below) ===================
    auto modalPanel = std::make_unique<engine::ui::UIVBox>();
    modalPanel->setPosition(glm::vec2((SCREEN_WIDTH - 320) / 2.0f, (SCREEN_HEIGHT - 200) / 2.0f));
    modalPanel->setSize(glm::vec2(320, 200));
    modalPanel->setPadding(glm::vec2(20));
    modalPanel->setSpacing(10);
    modalPanel->setInteractable(true);
    modalPanel->setStyleName("Panel");
    modalPanel->setVisible(false);  // hidden until an item slot is clicked
    
    auto modalTitle = std::make_unique<engine::ui::UILabel>();
    modalTitle->setText("Item Details");
    modalTitle->setSize(glm::vec2(280, 26));
    modalTitle->setHeading(true);
    modalTitle->setInteractable(true);
    engine::ui::UILabel* modalTitlePtr = modalTitle.get();
    modalPanel->addChild(std::move(modalTitle));
    
    auto modalDesc = std::make_unique<engine::ui::UILabel>();
    modalDesc->setText("Select an item from the inventory.");
    modalDesc->setSize(glm::vec2(280, 60));
    modalDesc->setInteractable(true);
    engine::ui::UILabel* modalDescPtr = modalDesc.get();
    modalPanel->addChild(std::move(modalDesc));
    
    auto modalCloseButton = std::make_unique<engine::ui::UIButton>();
    modalCloseButton->setText("Close");
    modalCloseButton->setSize(glm::vec2(90, 32));
    modalCloseButton->setStyleName("Button.Ghost");
    modalCloseButton->setInteractable(true);
    engine::ui::UIVBox* modalPanelRaw = modalPanel.get();
    modalCloseButton->onClick = [modalPanelRaw]() {
        modalPanelRaw->setVisible(false);
    };
    modalPanel->addChild(std::move(modalCloseButton));
    
    modalLayer->addComponent(modalPanelRaw);
    modalComponents.push_back(std::move(modalPanel));
    
    // === Top-left status panel (autoSize VBox, like the settings demo's
    // sections -- proves that pattern isn't special-cased to one screen) ==
    auto statusPanel = std::make_unique<engine::ui::UIVBox>();
    statusPanel->setPosition(glm::vec2(20, 20));
    statusPanel->setSpacing(6);
    statusPanel->setPadding(glm::vec2(14));
    statusPanel->setAutoSize(true);
    statusPanel->setInteractable(true);
    statusPanel->setStyleName("Panel");
    
    auto statusTitle = std::make_unique<engine::ui::UILabel>();
    statusTitle->setText("Status");
    statusTitle->setSize(glm::vec2(180, 26));
    statusTitle->setHeading(true);
    statusTitle->setInteractable(true);
    statusPanel->addChild(std::move(statusTitle));
    
    auto goldLabel = std::make_unique<engine::ui::UILabel>();
    goldLabel->setText("Gold: 250");
    goldLabel->setSize(glm::vec2(180, 20));
    goldLabel->setInteractable(true);
    statusPanel->addChild(std::move(goldLabel));
    
    auto healthLabel = std::make_unique<engine::ui::UILabel>();
    healthLabel->setText("Health");
    healthLabel->setSize(glm::vec2(180, 18));
    healthLabel->setInteractable(true);
    statusPanel->addChild(std::move(healthLabel));
    
    // Leave room below the labels for the bars, which are added as
    // independent top-level components below (not children of this
    // VBox) since two UIImages need to *overlap* (background + fill),
    // and a Box container can only stack children, never overlay them.
    auto barSpacer = std::make_unique<engine::ui::UILabel>();
    barSpacer->setText("");
    barSpacer->setSize(glm::vec2(180, 18));
    barSpacer->setCustomMinimumSize(glm::vec2(180, 18));
    barSpacer->setInteractable(true);
    statusPanel->addChild(std::move(barSpacer));
    
    auto manaLabel = std::make_unique<engine::ui::UILabel>();
    manaLabel->setText("Mana");
    manaLabel->setSize(glm::vec2(180, 18));
    manaLabel->setInteractable(true);
    statusPanel->addChild(std::move(manaLabel));
    
    auto manaBarSpacer = std::make_unique<engine::ui::UILabel>();
    manaBarSpacer->setText("");
    manaBarSpacer->setSize(glm::vec2(180, 18));
    manaBarSpacer->setCustomMinimumSize(glm::vec2(180, 18));
    manaBarSpacer->setInteractable(true);
    statusPanel->addChild(std::move(manaBarSpacer));
    
    // Force an immediate layout pass so statusPanel actually resizes
    // itself to its real content height *now*, before anything else on
    // screen is positioned relative to it. Without this, positioning
    // inventoryPanel below it required guessing that height by hand --
    // which is exactly the class of bug this whole project has been
    // fixing elsewhere (a hardcoded position drifting out of sync with
    // real auto-sized content), just freshly reintroduced here.
    statusPanel->layout();
    float statusPanelHeight = statusPanel->getSize().y;
    
    uiLayer->addComponent(statusPanel.get());
    engine::ui::UIComponent* statusPanelRaw = statusPanel.get();
    uiComponents.push_back(std::move(statusPanel));
    
    // Health/mana bars: raw UIImage pairs (dark background + accent fill),
    // positioned by hand relative to the status panel. These are added
    // straight to the "UI" layer as their own top-level siblings -- the
    // first time this framework has ever had more than one top-level
    // component on a single layer.
    auto addBar = [&](float yOffset, float fillFraction, glm::vec4 fillColor) {
        glm::vec2 basePos = statusPanelRaw->getWorldPosition() + glm::vec2(14.0f, yOffset);
        const float barWidth = 180.0f;
        const float barHeight = 14.0f;
        
        auto bg = std::make_unique<engine::ui::UIImage>();
        bg->setPosition(basePos);
        bg->setSize(glm::vec2(barWidth, barHeight));
        bg->setTintColor(glm::vec4(0.08f, 0.08f, 0.10f, 1.0f));
        bg->setInteractable(false);
        uiLayer->addComponent(bg.get());
        uiComponents.push_back(std::move(bg));
        
        auto fill = std::make_unique<engine::ui::UIImage>();
        fill->setPosition(basePos);
        fill->setSize(glm::vec2(barWidth * fillFraction, barHeight));
        fill->setTintColor(fillColor);
        fill->setInteractable(false);
        uiLayer->addComponent(fill.get());
        uiComponents.push_back(std::move(fill));
    };
    // Positions are hand-measured against statusPanel's own padding/spacing
    // above (title 26 + gold 20 + "Health" 18, each +6 spacing, +14 top
    // padding) -- a real game would use a proper progress-bar component
    // instead of eyeballed offsets like this; it's intentionally left this
    // way here specifically to stress-test raw absolute positioning.
    addBar(14.0f + 26.0f + 6.0f + 20.0f + 6.0f + 18.0f + 4.0f, 0.72f, glm::vec4(0.75f, 0.28f, 0.30f, 1.0f));   // health, red-ish
    addBar(14.0f + 26.0f + 6.0f + 20.0f + 6.0f + 18.0f + 6.0f + 18.0f + 6.0f + 18.0f + 4.0f, 0.45f, glm::vec4(0.35f, 0.45f, 0.85f, 1.0f)); // mana, blue-ish
    
    // === Left inventory panel geometry (declared up here so the hotbar,
    // built below, can position itself clear of this column instead of
    // centering blindly across the full window width and ending up
    // partly hidden behind it). ==========================================
    const float kInventoryX = 20.0f;
    const float kInventoryWidth = 260.0f;
    const float kInventoryY = 20.0f + statusPanelHeight + 20.0f;  // below status panel's real height
    
    // === Bottom hotbar: fixed-size UIHBox (not autoSize) with 8 toggle-
    // mode buttons, radio-style (only one selected at a time) ===========
    const int kHotbarSlots = 8;
    const float kSlotSize = 56.0f;
    auto hotbar = std::make_unique<engine::ui::UIHBox>();
    hotbar->setSpacing(8);
    hotbar->setPadding(glm::vec2(10));
    hotbar->setSize(glm::vec2(kHotbarSlots * (kSlotSize + 8) + 12, kSlotSize + 20));
    float hotbarWidth = kHotbarSlots * (kSlotSize + 8) + 12;
    // Right-anchored with a fixed margin instead of centered in the
    // "remaining space right of the inventory column" -- that centering
    // math checks out on paper (x: 328-852, comfortably inside a 900px
    // window) but produced an overflow in practice, and I couldn't find
    // why by re-reading it again. Anchoring to a fixed edge margin is a
    // strictly simpler computation with fewer places for the two of us
    // to disagree about what's happening, and it's still clear of the
    // inventory column (900-524-20=356 > 280, the inventory's right
    // edge) with real margin to spare.
    hotbar->setPosition(glm::vec2(SCREEN_WIDTH - hotbarWidth - 20.0f, SCREEN_HEIGHT - (kSlotSize + 20) - 20));
    hotbar->setInteractable(true);
    hotbar->setStyleName("Panel");
    
    auto hotbarButtons = std::make_shared<std::vector<engine::ui::UIButton*>>();
    for (int i = 1; i <= kHotbarSlots; ++i) {
        auto slot = std::make_unique<engine::ui::UIButton>();
        slot->setText(std::to_string(i));
        slot->setSize(glm::vec2(kSlotSize, kSlotSize));
        // Without this, UIButton::calculateMinSize()'s default 80px-wide
        // floor would win out over the 56px setSize() above in UIHBox's
        // max(getSize(), calculateMinSize()) measurement pass -- this is
        // what was actually causing the hotbar overflow.
        slot->setCustomMinimumSize(glm::vec2(kSlotSize, kSlotSize));
        slot->setToggleMode(true);
        slot->setInteractable(true);
        engine::ui::UIButton* slotRaw = slot.get();
        hotbarButtons->push_back(slotRaw);
        slot->onClick = [hotbarButtons, slotRaw]() {
            // Radio behavior: selecting one slot deselects every other.
            for (auto* btn : *hotbarButtons) {
                if (btn != slotRaw) {
                    btn->setToggled(false);
                }
            }
        };
        hotbar->addChild(std::move(slot));
    }
    
    uiLayer->addComponent(hotbar.get());
    uiComponents.push_back(std::move(hotbar));
    
    // === Left inventory panel: UIScrollContainer wrapping a UIGrid of 24
    // interactive slots (buttons, not static swatches) -- enough that
    // vertical scrolling is required, and a genuinely new Grid-inside-
    // Scroll combination the settings demo's grid never exercised. ======
    auto inventoryPanel = std::make_unique<engine::ui::UIScrollContainer>();
    inventoryPanel->setPosition(glm::vec2(kInventoryX, kInventoryY));
    inventoryPanel->setSize(glm::vec2(kInventoryWidth, SCREEN_HEIGHT - kInventoryY - 20.0f));
    inventoryPanel->setPadding(glm::vec2(14));
    inventoryPanel->setInteractable(true);
    inventoryPanel->setStyleName("Panel");
    
    auto inventoryContent = std::make_unique<engine::ui::UIVBox>();
    inventoryContent->setSpacing(10);
    inventoryContent->setAutoSize(true);
    inventoryContent->setInteractable(true);
    inventoryContent->setStyleName("Transparent");
    
    auto inventoryTitle = std::make_unique<engine::ui::UILabel>();
    inventoryTitle->setText("Inventory");
    inventoryTitle->setSize(glm::vec2(220, 26));
    inventoryTitle->setHeading(true);
    inventoryTitle->setInteractable(true);
    inventoryContent->addChild(std::move(inventoryTitle));
    
    auto inventoryGrid = std::make_unique<engine::ui::UIGrid>();
    const int kGridColumns = 3;
    const int kItemCount = 24;
    inventoryGrid->setColumns(kGridColumns);
    inventoryGrid->setRows((kItemCount + kGridColumns - 1) / kGridColumns);
    inventoryGrid->setCellWidth(64.0f);
    inventoryGrid->setCellHeight(64.0f);
    inventoryGrid->setSpacing(8.0f);
    inventoryGrid->setInteractable(true);
    inventoryGrid->setStyleName("Section");
    
    static const char* kItemNames[kItemCount] = {
        "Rusty Sword", "Wooden Shield", "Health Potion", "Mana Potion",
        "Iron Helmet", "Leather Boots", "Silver Ring", "Old Map",
        "Torch", "Rope", "Lockpick", "Bread",
        "Water Flask", "Fire Scroll", "Ice Scroll", "Gold Coin Pouch",
        "Steel Dagger", "Chainmail", "Wooden Bow", "Arrow Bundle",
        "Antidote", "Gem Shard", "Ancient Key", "Traveler's Cloak"
    };
    static const char* kItemDescs[kItemCount] = {
        "A worn blade, still sharp enough to matter.",
        "Splintered but sturdy. Blocks most things.",
        "Restores a modest amount of health.",
        "Restores a modest amount of mana.",
        "Dented, but it'll stop a glancing blow.",
        "Quiet enough for sneaking past a guard.",
        "Faintly warm to the touch. Unidentified.",
        "Half the ink has faded with age.",
        "Burns for about an hour.",
        "50 feet, slightly frayed at one end.",
        "Good for locks, bad for consciences.",
        "A day old, still edible.",
        "Enough for a short journey.",
        "Single use. Handle with care.",
        "Single use. Handle with more care.",
        "Heavier than it looks.",
        "Fast, but fragile in a real fight.",
        "Heavy. Slows you down but stops most cuts.",
        "Needs a matching bowstring.",
        "A dozen, fletched with crow feathers.",
        "Cures most common poisons.",
        "Catches the light strangely.",
        "Opens something. Unclear what.",
        "Weathered, but warm."
    };
    
    for (int i = 0; i < kItemCount; ++i) {
        auto slot = std::make_unique<engine::ui::UIButton>();
        slot->setText(std::to_string(i + 1));
        slot->setInteractable(true);
        const char* name = kItemNames[i];
        const char* desc = kItemDescs[i];
        slot->onClick = [modalPanelRaw, modalTitlePtr, modalDescPtr, name, desc]() {
            modalTitlePtr->setText(name);
            modalDescPtr->setText(desc);
            modalPanelRaw->setVisible(true);
        };
        inventoryGrid->addChild(std::move(slot));
    }
    
    inventoryContent->addChild(std::move(inventoryGrid));
    inventoryPanel->addChild(std::move(inventoryContent));
    
    uiLayer->addComponent(inventoryPanel.get());
    uiComponents.push_back(std::move(inventoryPanel));
    
    // Note: no explicit pre-loop layout pass is needed here -- the main
    // loop's uiManager.update(0.016) call, run before the first render,
    // already triggers layerManager_.layoutAll() internally.
    
    // Main loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    running = false;
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseMotion(
                    static_cast<float>(event.motion.x), static_cast<float>(event.motion.y),
                    static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel)
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseButton(
                    static_cast<float>(event.button.x), static_cast<float>(event.button.y),
                    event.button.button, true
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseButton(
                    static_cast<float>(event.button.x), static_cast<float>(event.button.y),
                    event.button.button, false
                );
                uiManager.dispatchInput(inputEvent);
            } else if (event.type == SDL_MOUSEWHEEL) {
                int rawY = (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -event.wheel.y : event.wheel.y;
                float delta = rawY > 0 ? 1.0f : -1.0f;
                engine::ui::InputEvent inputEvent = engine::ui::InputEvent::mouseWheel(
                    static_cast<float>(event.wheel.mouseX), static_cast<float>(event.wheel.mouseY), delta
                );
                uiManager.dispatchInput(inputEvent);
            }
        }
        
        uiManager.update(0.016);
        
        SDL_SetRenderDrawColor(renderer, 6, 6, 8, 255);
        SDL_RenderClear(renderer);
        
        // UI layer content first, modal last so it draws on top.
        for (auto& comp : uiComponents) {
            uiRenderer.render(comp.get());
        }
        for (auto& comp : modalComponents) {
            uiRenderer.render(comp.get());
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
