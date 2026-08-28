#pragma once

#include <SDL.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "render/Color.h"
#include "render/Font.h"

namespace engine::render {

// Rasterizes text via a Font into an SDL_Texture and draws it — with a
// cache, keyed by (text content, color), so drawing the same string in
// the same color across many consecutive frames (an item's quantity, a
// weight readout — anything that doesn't change every single frame)
// reuses the same GPU texture instead of re-rasterizing and
// re-uploading it every frame. That's the standard, easy-to-fall-into
// SDL_ttf performance trap this exists specifically to avoid.
//
// One TextRenderer is meant to be shared across everything using one
// SDL_Renderer, the same way TextureCache is — construct once, pass it
// wherever text needs to be drawn.
//
// Cache lifetime: entries are never evicted. For this project's actual
// text — item names (a small, fixed set from the item database),
// quantities (a small bounded range), a handful of UI labels — the
// total distinct strings ever requested is naturally small, so an
// unbounded cache is a reasonable, simpler-than-necessary tradeoff for
// now, not an oversight. It stops being reasonable the moment something
// renders many genuinely unique, one-off strings — e.g. Milestone 12's
// planned floating damage numbers, where every hit is a new string that
// will never repeat — at which point this needs real eviction (an LRU,
// or a per-frame "text that draws every frame" pool that bypasses the
// cache entirely) rather than growing forever.
class TextRenderer {
public:
    explicit TextRenderer(SDL_Renderer* renderer) : renderer_(renderer) {}

    ~TextRenderer() { clear(); }

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Draws `text` in `font`, tinted `color`, with (x, y) as its
    // top-left corner. A no-op for an empty string (not an error).
    void draw(const Font& font, const std::string& text, int x, int y, Color color) {
        const CachedText& cached = getOrRasterize(font, text, color);
        if (!cached.texture) return; // empty string

        SDL_FRect dest{static_cast<float>(x), static_cast<float>(y), static_cast<float>(cached.width),
                        static_cast<float>(cached.height)};
        SDL_RenderCopyF(renderer_, cached.texture, nullptr, &dest);
    }

    // Pixel size this exact (font, text, color) would draw at, without
    // drawing it — for layout decisions before you know final position
    // (e.g. right-aligning a number, centering a label). Shares the
    // same cache/rasterization path as draw(), so measuring text you
    // then immediately draw doesn't rasterize it twice.
    void measure(const Font& font, const std::string& text, Color color, int& outWidth, int& outHeight) {
        const CachedText& cached = getOrRasterize(font, text, color);
        outWidth = cached.width;
        outHeight = cached.height;
    }

    // Frees every cached texture. Not called automatically except by
    // the destructor — exists for a caller that knows it's about to
    // stop needing a large batch of one-off strings (see the class
    // comment's note on eviction) and wants to reclaim that GPU memory
    // explicitly, rather than growing the cache unboundedly.
    void clear() {
        for (auto& [key, cached] : cache_) {
            if (cached.texture) SDL_DestroyTexture(cached.texture);
        }
        cache_.clear();
    }

private:
    struct CachedText {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    static std::string cacheKey(const std::string& text, Color color) {
        // \x1f (unit separator) as a delimiter — vanishingly unlikely
        // to appear in real UI text, unlike a printable character.
        return text + '\x1f' + std::to_string(color.r) + ',' + std::to_string(color.g) + ',' +
               std::to_string(color.b) + ',' + std::to_string(color.a);
    }

    const CachedText& getOrRasterize(const Font& font, const std::string& text, Color color) {
        std::string key = cacheKey(text, color);
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        if (text.empty()) {
            return cache_.emplace(key, CachedText{nullptr, 0, 0}).first->second;
        }

        SDL_Color sdlColor{color.r, color.g, color.b, color.a};
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font.handle(), text.c_str(), sdlColor);
        if (!surface) {
            throw std::runtime_error(std::string("Failed to rasterize text '") + text + "': " + TTF_GetError());
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        int width = surface->w;
        int height = surface->h;
        SDL_FreeSurface(surface);

        if (!texture) {
            throw std::runtime_error(std::string("Failed to create texture from rasterized text: ") +
                                      SDL_GetError());
        }

        return cache_.emplace(key, CachedText{texture, width, height}).first->second;
    }

    SDL_Renderer* renderer_;
    std::unordered_map<std::string, CachedText> cache_;
};

} // namespace engine::render
