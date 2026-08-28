#pragma once

#include <algorithm>
#include <vector>

#include <SDL.h>
#include <glm/vec2.hpp>

#include "render/Camera.h"
#include "render/Color.h"

namespace engine::fx {

// Generalizes the Milestone 5 hitscan tracer (a single hand-rolled
// start/end/timer triple in main.cpp) into a small pool — not tied to
// a specific shooter or even to combat. trigger() any time a
// short-lived world-space line should flash (a bullet path, a laser
// sight, a debug ray); this owns its own lifetime/fade independently
// of the ECS, since a tracer isn't really "an entity," it's a visual
// event.
class WorldLineEffects {
public:
    void trigger(glm::vec2 start, glm::vec2 end, render::Color color, double duration) {
        active_.push_back(Line{start, end, color, duration, duration});
    }

    // Advances every active line's timer and drops any that finished.
    // Call once per fixed update.
    void tick(double dt) {
        for (auto& line : active_) line.timeRemaining -= dt;
        active_.erase(std::remove_if(active_.begin(), active_.end(),
                                      [](const Line& l) { return l.timeRemaining <= 0.0; }),
                      active_.end());
    }

    // Call once per render, after SpriteRenderSystem::render() so lines
    // draw on top of sprites — matches where the Milestone 5 tracer drew.
    void render(SDL_Renderer* renderer, const render::Camera& camera) const {
        if (active_.empty()) return;

        SDL_BlendMode previousBlendMode;
        SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        for (const auto& line : active_) {
            double sx1 = 0.0, sy1 = 0.0, sx2 = 0.0, sy2 = 0.0;
            camera.worldToScreen(line.start.x, line.start.y, sx1, sy1);
            camera.worldToScreen(line.end.x, line.end.y, sx2, sy2);

            double fade = line.duration > 0.0 ? line.timeRemaining / line.duration : 0.0;
            auto alpha = static_cast<Uint8>(255.0 * fade);
            SDL_SetRenderDrawColor(renderer, line.color.r, line.color.g, line.color.b, alpha);
            SDL_RenderDrawLineF(renderer, static_cast<float>(sx1), static_cast<float>(sy1),
                                 static_cast<float>(sx2), static_cast<float>(sy2));
        }

        SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
    }

private:
    struct Line {
        glm::vec2 start;
        glm::vec2 end;
        render::Color color;
        double duration;
        double timeRemaining;
    };
    std::vector<Line> active_;
};

} // namespace engine::fx
