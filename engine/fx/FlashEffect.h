#pragma once

#include "ecs/Registry.h"
#include "render/Color.h"
#include "render/Sprite.h"

namespace engine::fx {

// Attach to any entity whose Sprite.color should be entirely owned by
// this effect: baseColor the rest of the time, flashColor for
// `duration` seconds after trigger() is called. Generalizes the
// Milestone 5 muzzle-flash pattern (a hand-checked timer variable,
// with Sprite.color written directly in main.cpp's render callback)
// so the next entity that needs a flash — an NPC's weapon, a hit-flash
// on taking damage — doesn't mean copy-pasting that pattern again.
//
// Note this is specifically for a two-state (base/flash) timed swap.
// The Milestone 5 target dummy's damage-darken tint is a different
// shape of problem — a continuous function of a Health value, not a
// timed two-state flash — and deliberately stays as game-level code
// in main.cpp rather than being forced into this component.
struct FlashEffect {
    render::Color baseColor{255, 255, 255, 255};
    render::Color flashColor{255, 255, 255, 255};
    double duration = 0.06;
    double timeRemaining = 0.0; // > 0 while actively flashing

    void trigger() { timeRemaining = duration; }
};

class FlashSystem {
public:
    // Advances every FlashEffect's timer. Call once per fixed update.
    static void tick(ecs::Registry& registry, double dt) {
        for (ecs::Entity e : registry.view<FlashEffect>()) {
            auto& flash = registry.get<FlashEffect>(e);
            if (flash.timeRemaining > 0.0) {
                flash.timeRemaining -= dt;
                if (flash.timeRemaining < 0.0) flash.timeRemaining = 0.0;
            }
        }
    }

    // Writes Sprite.color for every FlashEffect-bearing entity this
    // frame. Call once per render, before SpriteRenderSystem::render().
    static void apply(ecs::Registry& registry) {
        for (ecs::Entity e : registry.view<FlashEffect, render::Sprite>()) {
            const auto& flash = registry.get<FlashEffect>(e);
            auto& sprite = registry.get<render::Sprite>(e);
            sprite.color = flash.timeRemaining > 0.0 ? flash.flashColor : flash.baseColor;
        }
    }
};

} // namespace engine::fx
