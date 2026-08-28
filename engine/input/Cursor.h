#pragma once

#include <SDL.h>

#include "render/Color.h"

namespace engine::input {

// Minimal custom-cursor capability: hides the OS cursor and draws a
// texture (any SDL_Texture* — including TextureCache::whitePixel() for
// a plain tinted-rectangle placeholder, the same pattern every other
// placeholder in this codebase already uses) at the mouse position
// instead. Building the *capability* is all Milestone 5.5 scopes —
// deciding *when* to swap cursors (default vs. weapon-specific vs.
// showing the OS cursor while an inventory panel is open) needs a real
// trigger condition to hook into, and most of those don't exist yet:
// Milestone 6's UI is the first real caller for the inventory case.
class Cursor {
public:
    // Hides the OS cursor and switches to drawing `texture` (tinted by
    // `color`, sized width x height, centered on the mouse position)
    // every render() call until changed again.
    void set(SDL_Texture* texture, render::Color color, int width, int height) {
        texture_ = texture;
        color_ = color;
        width_ = width;
        height_ = height;
        SDL_ShowCursor(SDL_DISABLE);
    }

    // Restores the OS cursor and stops drawing anything here.
    void showSystemCursor() {
        texture_ = nullptr;
        SDL_ShowCursor(SDL_ENABLE);
    }

    // Call after all world/UI rendering, so the custom cursor draws on
    // top of everything else — the same ordering SDL's own hardware
    // cursor would use.
    void render(SDL_Renderer* renderer, int mouseX, int mouseY) const {
        if (!texture_) return;

        SDL_SetTextureColorMod(texture_, color_.r, color_.g, color_.b);
        SDL_SetTextureAlphaMod(texture_, color_.a);

        SDL_FRect dest;
        dest.x = static_cast<float>(mouseX) - width_ * 0.5f;
        dest.y = static_cast<float>(mouseY) - height_ * 0.5f;
        dest.w = static_cast<float>(width_);
        dest.h = static_cast<float>(height_);
        SDL_RenderCopyF(renderer, texture_, nullptr, &dest);
    }

private:
    SDL_Texture* texture_ = nullptr;
    render::Color color_{255, 255, 255, 255};
    int width_ = 0;
    int height_ = 0;
};

} // namespace engine::input
