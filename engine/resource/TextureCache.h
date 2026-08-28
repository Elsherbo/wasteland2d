#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace engine::resource {

// One texture cache per renderer (textures are renderer-bound in SDL).
// Milestone 2 hands out a shared 1x1 white pixel for colored-rectangle
// placeholders — that path is unchanged and still the default (see
// Sprite.h). Milestone 5.5 adds real PNG loading via load(path),
// cached by path so the same image is never loaded twice: this is now
// the single texture-owning path in the engine — TileMap's tileset
// image loads through here too (Milestone 5.5), instead of loading and
// owning its own SDL_Texture independently the way it did through
// Milestone 4.
class TextureCache {
public:
    explicit TextureCache(SDL_Renderer* renderer) : renderer_(renderer) {
        whitePixel_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC, 1, 1);
        if (!whitePixel_) {
            throw std::runtime_error(std::string("Failed to create white pixel texture: ") +
                                      SDL_GetError());
        }
        SDL_SetTextureBlendMode(whitePixel_, SDL_BLENDMODE_BLEND);

        Uint32 white = 0xFFFFFFFFu;
        SDL_UpdateTexture(whitePixel_, nullptr, &white, sizeof(white));
    }

    ~TextureCache() {
        if (whitePixel_) SDL_DestroyTexture(whitePixel_);
        for (auto& [path, texture] : cache_) {
            SDL_DestroyTexture(texture);
        }
    }

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    SDL_Texture* whitePixel() const { return whitePixel_; }

    // Loads (and caches by path) a real texture via SDL2_image. Same
    // image requested twice returns the same SDL_Texture* rather than
    // loading it again. Ownership stays here — callers store the
    // returned pointer but never destroy it themselves; it's freed in
    // ~TextureCache() alongside whitePixel_.
    SDL_Texture* load(const std::string& path) {
        auto it = cache_.find(path);
        if (it != cache_.end()) return it->second;

        SDL_Texture* texture = IMG_LoadTexture(renderer_, path.c_str());
        if (!texture) {
            throw std::runtime_error("Failed to load texture '" + path + "': " + IMG_GetError());
        }

        cache_.emplace(path, texture);
        return texture;
    }

private:
    SDL_Renderer* renderer_;
    SDL_Texture* whitePixel_ = nullptr;
    std::unordered_map<std::string, SDL_Texture*> cache_;
};

} // namespace engine::resource
