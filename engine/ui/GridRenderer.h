#pragma once

#include <SDL.h>

#include "render/Color.h"
#include "ui/GridLayout.h"

namespace engine::ui {

// Draws every cell of a grid as a filled, bordered square — the empty
// "slots" look of any grid-based panel, independent of what's actually
// stored in the cells (that's game/ui/InventoryRenderer.h's job — it
// draws item stacks on top of this).
inline void renderGridCells(SDL_Renderer* renderer, const GridLayout& layout, render::Color background,
                             render::Color border) {
    for (int y = 0; y < layout.gridHeight; ++y) {
        for (int x = 0; x < layout.gridWidth; ++x) {
            int px = 0, py = 0, pw = 0, ph = 0;
            layout.cellRect(x, y, px, py, pw, ph);
            SDL_FRect rect{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw),
                            static_cast<float>(ph)};

            SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
            SDL_RenderFillRectF(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_RenderDrawRectF(renderer, &rect);
        }
    }
}

// Draws a translucent fill + border over a gridX/gridY, width x height
// footprint — used for hover highlighting (a plain cell, or a whole
// stack's footprint under the cursor) and drag drop-previews (tinted
// green/red for a valid/invalid drop; see game::ui for how that color
// choice is made — this function only draws whatever color it's given,
// same "generic primitive, game code decides meaning" split as the
// rest of engine::ui).
inline void renderHighlightRect(SDL_Renderer* renderer, const GridLayout& layout, int gridX, int gridY, int width,
                                 int height, render::Color color) {
    int px = 0, py = 0, pw = 0, ph = 0;
    layout.cellRect(gridX, gridY, px, py, pw, ph);
    SDL_FRect rect{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw * width),
                    static_cast<float>(ph * height)};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    SDL_RenderDrawRectF(renderer, &rect);
}

} // namespace engine::ui
