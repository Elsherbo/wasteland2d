#pragma once

#include <optional>
#include <utility>

namespace engine::ui {

// Pure screen-space <-> grid-cell math for a fixed-position, square-cell
// grid panel — no dependency on what the grid actually represents
// (inventory, crafting, a hotbar, ...), which is what keeps this at
// engine/ level while the things that actually reference
// game::components::Inventory stay in game/ui/.
struct GridLayout {
    int screenX = 0;
    int screenY = 0;
    int cellSize = 32;
    int gridWidth = 0;
    int gridHeight = 0;

    // The cell the given screen-space point falls within, or
    // std::nullopt if it's outside this grid's bounds entirely.
    // Explicit bounds checks rather than relying on integer division
    // alone — division floors toward zero, which is wrong for a point
    // above/left of the grid (negative local coordinates).
    std::optional<std::pair<int, int>> cellAt(int screenPointX, int screenPointY) const {
        int localX = screenPointX - screenX;
        int localY = screenPointY - screenY;
        if (localX < 0 || localY < 0) return std::nullopt;

        int cx = localX / cellSize;
        int cy = localY / cellSize;
        if (cx >= gridWidth || cy >= gridHeight) return std::nullopt;
        return std::make_pair(cx, cy);
    }

    // Screen-space rect for a given cell — for rendering. Does not
    // validate that (gridX, gridY) is actually within bounds; callers
    // that got the position from cellAt() already know it is.
    void cellRect(int gridX, int gridY, int& outX, int& outY, int& outW, int& outH) const {
        outX = screenX + gridX * cellSize;
        outY = screenY + gridY * cellSize;
        outW = cellSize;
        outH = cellSize;
    }

    // True if the given screen point falls anywhere within this grid's
    // total rectangle (not necessarily a specific cell — same thing as
    // cellAt().has_value(), provided as a named check for readability
    // at call sites that only care about "is the mouse over this panel
    // at all", e.g. choosing which of several visible panels a drop
    // target belongs to).
    bool contains(int screenPointX, int screenPointY) const { return cellAt(screenPointX, screenPointY).has_value(); }
};

} // namespace engine::ui
