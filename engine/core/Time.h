#pragma once

#include <cstdint>

namespace engine {

// Drives a fixed-timestep simulation with a variable-rate render.
// update() is called a deterministic number of times per second
// (60 by default) regardless of how fast/slow the machine renders;
// render() can then interpolate using alpha() for smooth visuals.
//
// This matters a lot for a survival sim: hunger/radiation ticking,
// AI decisions, and physics all need to behave identically whether
// the game runs at 30fps or 240fps.
class Clock {
public:
    explicit Clock(double fixedHz = 60.0)
        : fixedDt_(1.0 / fixedHz) {}

    // Call once per frame with the current high-resolution time (seconds).
    // Returns how many fixed steps should run this frame via shouldStep(),
    // called in a while-loop by the owner (see Application::run).
    void beginFrame(double nowSeconds) {
        if (lastTime_ < 0.0) {
            lastTime_ = nowSeconds;
        }
        double frameTime = nowSeconds - lastTime_;
        lastTime_ = nowSeconds;

        // Clamp to avoid a "spiral of death" after a debugger pause
        // or a dropped frame (e.g. alt-tabbing).
        constexpr double kMaxFrameTime = 0.25;
        if (frameTime > kMaxFrameTime) {
            frameTime = kMaxFrameTime;
        }

        accumulator_ += frameTime;
        realDeltaTime_ = frameTime;
    }

    // Pop one fixed step off the accumulator if enough time has built up.
    bool shouldStep() {
        if (accumulator_ >= fixedDt_) {
            accumulator_ -= fixedDt_;
            return true;
        }
        return false;
    }

    // 0..1 — how far between the last fixed step and the next one we are,
    // for render-side interpolation of transforms.
    double alpha() const { return accumulator_ / fixedDt_; }

    double fixedDeltaTime() const { return fixedDt_; }
    double realDeltaTime() const { return realDeltaTime_; }

private:
    double fixedDt_;
    double accumulator_ = 0.0;
    double lastTime_ = -1.0;
    double realDeltaTime_ = 0.0;
};

} // namespace engine
