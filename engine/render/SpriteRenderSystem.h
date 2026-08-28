#pragma once

#include <algorithm>
#include <vector>

#include "core/Window.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"
#include "render/Camera.h"
#include "render/Sprite.h"
#include "resource/TextureCache.h"

namespace engine::render {

class SpriteRenderSystem {
public:
    // Pulled out from render() so it's testable without an SDL renderer
    // or a window — draw-order correctness (layer first, then Y-sort
    // within a layer) is real logic worth verifying on its own, not
    // just eyeballing on screen.
    static std::vector<ecs::Entity> computeDrawOrder(ecs::Registry& registry) {
        auto entities = registry.view<ecs::Transform, Sprite>();

        // Two-key stable sort: layer first (coarse bands — ground below
        // dynamic objects below UI), then world Y position within a
        // shared layer. The Y-sort is what makes the player walk behind
        // a tall object when above it and in front when below — draw
        // order flips the instant the player's Y crosses the object's
        // sort point. sortOriginYOffset lets a tall sprite push its sort
        // point toward its visual base instead of its center.
        std::stable_sort(entities.begin(), entities.end(), [&registry](ecs::Entity a, ecs::Entity b) {
            const auto& spriteA = registry.get<Sprite>(a);
            const auto& spriteB = registry.get<Sprite>(b);
            if (spriteA.layer != spriteB.layer) return spriteA.layer < spriteB.layer;

            float sortYA = registry.get<ecs::Transform>(a).y + spriteA.sortOriginYOffset;
            float sortYB = registry.get<ecs::Transform>(b).y + spriteB.sortOriginYOffset;
            return sortYA < sortYB;
        });

        return entities;
    }

    static void render(ecs::Registry& registry, Window& window,
                        resource::TextureCache& textures, const Camera& camera) {
        auto entities = computeDrawOrder(registry);
        SDL_Renderer* renderer = window.renderer();

        for (ecs::Entity e : entities) {
            const auto& transform = registry.get<ecs::Transform>(e);
            const auto& sprite = registry.get<Sprite>(e);

            double screenX, screenY;
            camera.worldToScreen(transform.x, transform.y, screenX, screenY);

            double w = sprite.width * transform.scale * camera.zoom();
            double h = sprite.height * transform.scale * camera.zoom();

            SDL_FRect dest;
            dest.x = static_cast<float>(screenX - w * 0.5);
            dest.y = static_cast<float>(screenY - h * 0.5);
            dest.w = static_cast<float>(w);
            dest.h = static_cast<float>(h);

            SDL_Texture* tex = sprite.texture ? sprite.texture : textures.whitePixel();
            SDL_SetTextureColorMod(tex, sprite.color.r, sprite.color.g, sprite.color.b);
            SDL_SetTextureAlphaMod(tex, sprite.color.a);

            // Only a real texture with a non-empty sourceRect selects a
            // sub-frame; whitePixel() (and any real texture used
            // whole) passes nullptr, same as every milestone before
            // this one — unchanged default behavior.
            const SDL_Rect* srcRect =
                (sprite.texture && sprite.sourceRect.w > 0 && sprite.sourceRect.h > 0)
                    ? &sprite.sourceRect
                    : nullptr;

            if (transform.rotationDegrees != 0.0) {
                SDL_RenderCopyExF(renderer, tex, srcRect, &dest, transform.rotationDegrees,
                                   nullptr, SDL_FLIP_NONE);
            } else {
                SDL_RenderCopyF(renderer, tex, srcRect, &dest);
            }
        }
    }
};

} // namespace engine::render
