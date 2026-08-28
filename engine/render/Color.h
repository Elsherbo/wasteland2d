#pragma once

#include <cstdint>

namespace engine::render {

// Deliberately not SDL_Color: components are engine-public API and
// shouldn't leak a third-party type just because the current renderer
// happens to be SDL. SpriteRenderSystem (which is already inherently
// SDL-coupled — it calls SDL_RenderCopy et al.) is where this gets
// converted to SDL_Color, not here.
struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

} // namespace engine::render
