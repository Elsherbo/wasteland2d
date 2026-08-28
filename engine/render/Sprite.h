#pragma once

#include <SDL.h>

#include "render/Color.h"

namespace engine::render {

// Milestone 4 adds sortOriginYOffset for Y-sorting: within a shared
// layer, draw order is by (Transform.y + sortOriginYOffset) ascending,
// not just layer. This is what lets the player walk behind a tall
// object when above it and in front when below — the offset exists so
// a tall sprite's *sort point* can be pushed toward its visual base
// rather than its vertical center (a tree's canopy shouldn't determine
// whether the player is "behind" it — the trunk's base should).
// Leave at 0 for anything that doesn't need this (most ground-level
// objects sort fine using Transform.y as-is).
//
// Milestone 5.5 adds an optional real texture: when `texture` is
// nullptr (the default), SpriteRenderSystem draws exactly what it
// always has — the shared tinted white-pixel rectangle, sized by
// width/height and tinted by color. Nothing about that path changed;
// every Milestone 2-5 placeholder (the target dummy, the vehicle
// placeholder, the tree) keeps working completely unchanged. Setting
// `texture` (via TextureCache::load()) switches that one sprite to
// drawing real art instead — `sourceRect` then selects a single frame
// out of an atlas/spritesheet (leave it default {0,0,0,0} to use the
// whole texture). color/width/height still apply either way: color
// becomes a tint (SDL texture color-mod) over the real art — which is
// what lets effects like the Milestone 5 damage-darken and
// muzzle-flash tricks keep working unchanged once a sprite has real
// art instead of being a plain rectangle.
struct Sprite {
    float width = 24.0f;
    float height = 24.0f;
    Color color{255, 255, 255, 255};
    int layer = 0;              // coarse band: ground < dynamic < UI
    float sortOriginYOffset = 0.0f;

    SDL_Texture* texture = nullptr;    // nullptr = colored-rectangle placeholder (unchanged behavior)
    SDL_Rect sourceRect{0, 0, 0, 0};   // frame within `texture`; all-zero = use the whole texture
};

} // namespace engine::render
