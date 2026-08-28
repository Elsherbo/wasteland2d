#pragma once

#include <string>

#include <SDL.h>

#include "render/Font.h"
#include "render/TextRenderer.h"
#include "ui/GridLayout.h"
#include "ui/GridRenderer.h"

#include "components/EquipmentSlots.h"
#include "components/QuickSlots.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"

namespace game::ui {

namespace detail {
inline std::string equippedItemId(const components::EquipmentSlots& slots, components::EquipmentSlots::Slot slot) {
    switch (slot) {
        case components::EquipmentSlots::Slot::Primary: return slots.primaryItemId;
        case components::EquipmentSlots::Slot::Secondary: return slots.secondaryItemId;
        case components::EquipmentSlots::Slot::Melee: return slots.meleeItemId;
        case components::EquipmentSlots::Slot::Backpack: return slots.backpackItemId;
    }
    return {};
}
} // namespace detail

// Draws the 4 equipment slot boxes (Primary, Secondary, Melee,
// Backpack, in that order) as a 4-wide, 1-tall row — `layout` is
// exactly that: reusing engine::ui::GridLayout/GridRenderer the same
// way the inventory grid panels do, since a row of fixed-size boxes is
// structurally identical to a 4x1 grid. Whichever of
// Primary/Secondary/Melee is currently active (Backpack is never
// "active" — see EquipmentSlots.h) gets a highlighted background, so
// it's visually clear which weapon is in-hand.
inline void renderEquipmentSlots(SDL_Renderer* renderer, const engine::ui::GridLayout& layout,
                                  const components::EquipmentSlots& slots, const data::ItemDatabase& db) {
    engine::ui::renderGridCells(renderer, layout, engine::render::Color{50, 48, 45, 230},
                                 engine::render::Color{100, 96, 90, 255});

    for (int i = 0; i < 4; ++i) {
        auto slot = static_cast<components::EquipmentSlots::Slot>(i);
        std::string itemId = detail::equippedItemId(slots, slot);

        if (slot == slots.activeSlot && slot != components::EquipmentSlots::Slot::Backpack) {
            engine::ui::renderHighlightRect(renderer, layout, i, 0, 1, 1, engine::render::Color{230, 210, 120, 70});
        }

        if (itemId.empty()) continue;
        const data::ItemDefinition* def = db.find(itemId);
        if (!def) continue;

        int px = 0, py = 0, pw = 0, ph = 0;
        layout.cellRect(i, 0, px, py, pw, ph);
        SDL_FRect inner{static_cast<float>(px + 4), static_cast<float>(py + 4), static_cast<float>(pw - 8),
                         static_cast<float>(ph - 8)};
        SDL_SetRenderDrawColor(renderer, def->iconColor.r, def->iconColor.g, def->iconColor.b, def->iconColor.a);
        SDL_RenderFillRectF(renderer, &inner);
    }
}

// The always-visible hotbar (keys 4-9, see QuickSlots) — drawn
// regardless of whether the inventory panel is open, same as any
// action-game quick-slot bar. Each bound slot shows a tinted swatch
// for its item plus a total-count readout (summed across every
// matching stack in playerInventory, not just one — see
// InventorySystem::totalQuantityOf).
inline void renderHotbar(SDL_Renderer* renderer, const engine::ui::GridLayout& layout,
                          const components::QuickSlots& quickSlots, const components::Inventory& playerInventory,
                          const data::ItemDatabase& db, const engine::render::Font& font,
                          engine::render::TextRenderer& textRenderer) {
    engine::ui::renderGridCells(renderer, layout, engine::render::Color{35, 34, 32, 210},
                                 engine::render::Color{90, 88, 84, 255});

    for (int i = 0; i < components::QuickSlots::kSlotCount; ++i) {
        const std::string& itemId = quickSlots.itemIds[static_cast<std::size_t>(i)];

        int px = 0, py = 0, pw = 0, ph = 0;
        layout.cellRect(i, 0, px, py, pw, ph);

        // Always draw the key number, bound or not — it's how the
        // player knows which physical key does what before they've
        // assigned anything to it.
        std::string keyLabel = std::to_string(i + 4);
        textRenderer.draw(font, keyLabel, px + 3, py + 2, engine::render::Color{150, 148, 140, 255});

        if (itemId.empty()) continue;
        const data::ItemDefinition* def = db.find(itemId);
        if (!def) continue;

        SDL_FRect inner{static_cast<float>(px + 4), static_cast<float>(py + 16), static_cast<float>(pw - 8),
                         static_cast<float>(ph - 20)};
        SDL_SetRenderDrawColor(renderer, def->iconColor.r, def->iconColor.g, def->iconColor.b, def->iconColor.a);
        SDL_RenderFillRectF(renderer, &inner);

        int total = systems::InventorySystem::totalQuantityOf(playerInventory, itemId);
        std::string countText = std::to_string(total);
        int textW = 0, textH = 0;
        textRenderer.measure(font, countText, engine::render::Color{255, 255, 255, 255}, textW, textH);
        textRenderer.draw(font, countText, px + pw - textW - 3, py + ph - textH - 2,
                           engine::render::Color{255, 255, 255, 255});
    }
}

} // namespace game::ui
