#pragma once

#include <random>

#include <glm/vec2.hpp>

namespace engine::fx {

// Formalizes the decay/jitter math Milestone 5 hand-rolled in main.cpp
// (shakeTimer, shakeMagnitude, a local std::mt19937 + distribution,
// and a falloff calculation) into one reusable object. Camera itself
// stays dumb (Camera::setShakeOffset just applies whatever offset it's
// handed) — this is what computes that offset over time.
class ScreenShake {
public:
    void trigger(double magnitude, double duration) {
        magnitude_ = magnitude;
        duration_ = duration;
        timeRemaining_ = duration;
    }

    // Advances the timer and returns this frame's camera offset — feed
    // straight into Camera::setShakeOffset(). Returns {0, 0} once the
    // shake has finished. Call once per fixed update.
    glm::vec2 tick(double dt) {
        if (timeRemaining_ <= 0.0) return glm::vec2(0.0f, 0.0f);

        timeRemaining_ -= dt;
        if (timeRemaining_ < 0.0) timeRemaining_ = 0.0;

        double falloff = duration_ > 0.0 ? timeRemaining_ / duration_ : 0.0;
        return glm::vec2(static_cast<float>(dist_(rng_) * magnitude_ * falloff),
                          static_cast<float>(dist_(rng_) * magnitude_ * falloff));
    }

private:
    double magnitude_ = 0.0;
    double duration_ = 0.0;
    double timeRemaining_ = 0.0;
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<double> dist_{-1.0, 1.0};
};

} // namespace engine::fx
