#pragma once

#include <cstdio>
#include <string>

#include <SDL.h>

#include "render/Font.h"
#include "render/TextRenderer.h"
#include "ui/GridLayout.h"

#include "components/Inventory.h"
#include "data/ItemDatabase.h"
#include "systems/InventorySystem.h"
#include "ui/DragDropController.h"

namespace game::ui {

// Draws every stack in `inventory` as a filled rectangle spanning its
// item's footprint, tinted by that item's ItemDefinition::iconColor —
// the same colored-rectangle placeholder convention every other visual
// in this codebase uses until real icon art exists.
inline void renderInventoryContents(SDL_Renderer* renderer, const engine::ui::GridLayout& layout,
                                     const components::Inventory& inventory, const data::ItemDatabase& db) {
    for (const auto& stack : inventory.stacks) {
        const data::ItemDefinition* def = db.find(stack.itemId);
        if (!def) continue;

        int effectiveW = stack.rotated ? def->height : def->width;
        int effectiveH = stack.rotated ? def->width : def->height;

        int px = 0, py = 0, pw = 0, ph = 0;
        layout.cellRect(stack.gridX, stack.gridY, px, py, pw, ph);

        SDL_FRect rect{static_cast<float>(px + 2), static_cast<float>(py + 2),
                        static_cast<float>(effectiveW * pw - 4), static_cast<float>(effectiveH * ph - 4)};
        SDL_SetRenderDrawColor(renderer, def->iconColor.r, def->iconColor.g, def->iconColor.b, def->iconColor.a);
        SDL_RenderFillRectF(renderer, &rect);
    }
}

// Draws each stack's quantity — only when > 1, so a plain single item
// doesn't get a redundant "1" cluttering it — in the bottom-right
// corner of its footprint. Call after renderInventoryContents() so the
// number draws on top of the item's tinted rectangle.
inline void renderStackQuantities(const engine::ui::GridLayout& layout, const components::Inventory& inventory,
                                   const data::ItemDatabase& db, const engine::render::Font& font,
                                   engine::render::TextRenderer& textRenderer) {
    static const engine::render::Color kQuantityColor{255, 255, 255, 255};

    for (const auto& stack : inventory.stacks) {
        if (stack.quantity <= 1) continue;
        const data::ItemDefinition* def = db.find(stack.itemId);
        if (!def) continue;

        int effectiveW = stack.rotated ? def->height : def->width;
        int effectiveH = stack.rotated ? def->width : def->height;
        int px = 0, py = 0, pw = 0, ph = 0;
        layout.cellRect(stack.gridX, stack.gridY, px, py, pw, ph);

        std::string text = std::to_string(stack.quantity);
        int textW = 0, textH = 0;
        textRenderer.measure(font, text, kQuantityColor, textW, textH);

        int textX = px + effectiveW * pw - textW - 3;
        int textY = py + effectiveH * ph - textH - 1;
        textRenderer.draw(font, text, textX, textY, kQuantityColor);
    }
}

// Draws a "used / max kg" readout just above a panel — e.g. "12.3 /
// 40.0 kg". Any inventory panel wants this, so it takes a GridLayout
// (for positioning) rather than being specific to the player's panel.
inline void renderWeightReadout(const engine::ui::GridLayout& layout, const components::Inventory& inventory,
                                 const data::ItemDatabase& db, const engine::render::Font& font,
                                 engine::render::TextRenderer& textRenderer) {
    float used = systems::InventorySystem::currentWeight(inventory, db);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.1f / %.1f kg", static_cast<double>(used),
                  static_cast<double>(inventory.maxWeight));

    engine::render::Color color =
        used > inventory.maxWeight - 0.05f ? engine::render::Color{230, 120, 90, 255} // near/at capacity
                                            : engine::render::Color{210, 208, 195, 255};
    textRenderer.draw(font, buf, layout.screenX, layout.screenY - 24, color);
}

// Draws an item's name near the cursor — a plain-text tooltip, no
// background panel or wrapping (a real tooltip widget with a
// backdrop/border is future polish; this is the minimum that makes
// "what am I looking at" answerable, which was the actual gap).
inline void renderItemNameTooltip(SDL_Renderer* renderer, const std::string& itemId, const data::ItemDatabase& db,
                                   const engine::render::Font& font, engine::render::TextRenderer& textRenderer,
                                   int mouseX, int mouseY) {
    const data::ItemDefinition* def = db.find(itemId);
    if (!def) return;

    int textW = 0, textH = 0;
    textRenderer.measure(font, def->name, engine::render::Color{255, 255, 255, 255}, textW, textH);

    int x = mouseX + 14;
    int y = mouseY + 14;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 15, 15, 18, 210);
    SDL_FRect backdrop{static_cast<float>(x - 4), static_cast<float>(y - 2), static_cast<float>(textW + 8),
                        static_cast<float>(textH + 4)};
    SDL_RenderFillRectF(renderer, &backdrop);

    textRenderer.draw(font, def->name, x, y, engine::render::Color{255, 255, 255, 255});
}

// Draws whatever DragDropController is currently holding, following
// the mouse — the drag "ghost". Anchored so the cell you actually
// grabbed (HeldStack::grabOffsetX/Y) stays under the cursor, tracking
// it smoothly in pixels as you move — not the item's top-left corner
// snapping to the cursor regardless of where you clicked it. The grid-
// snapped *landing* position (where it would actually go if dropped
// right now) is a separate concern, shown by the highlight/drop-
// preview rendering in main.cpp via DragDropController::
// resolveDropTopLeft(), not by this ghost. Call after
// renderInventoryContents() for every visible panel, so the dragged
// item always draws on top.
inline void renderHeldStack(SDL_Renderer* renderer, const data::ItemDatabase& db, const DragDropController& drag,
                             int cellSize, int mouseX, int mouseY) {
    if (!drag.isDragging()) return;

    const HeldStack& held = drag.held();
    const data::ItemDefinition* def = db.find(held.stack.itemId);
    if (!def) return;

    int effectiveW = held.stack.rotated ? def->height : def->width;
    int effectiveH = held.stack.rotated ? def->width : def->height;

    SDL_FRect rect{static_cast<float>(mouseX - held.grabOffsetX * cellSize),
                    static_cast<float>(mouseY - held.grabOffsetY * cellSize),
                    static_cast<float>(effectiveW * cellSize), static_cast<float>(effectiveH * cellSize)};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, def->iconColor.r, def->iconColor.g, def->iconColor.b, 180);
    SDL_RenderFillRectF(renderer, &rect);
}

} // namespace game::ui
