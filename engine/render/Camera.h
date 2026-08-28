#pragma once

#include <cmath>

namespace engine::render {

// World-space position of the camera's center, plus zoom. All world<->
// screen conversion for rendering and (later) mouse-picking goes through
// here, so nothing else needs to know how the transform works.
class Camera {
public:
    Camera(int viewportWidth, int viewportHeight)
        : viewportWidth_(viewportWidth), viewportHeight_(viewportHeight) {}

    void setPosition(double x, double y) { x_ = x; y_ = y; }
    void setZoom(double zoom) { zoom_ = zoom; }

    // Exponential smoothing toward a target zoom level — same technique
    // as followSmooth, applied to zoom_ instead of position. Used for
    // aim-down-sights zoom: smooth in when ADS starts, smooth back to
    // 1.0 when it ends, rather than an instant zoom snap. Call every
    // fixed update with whatever target zoom applies this frame (1.0
    // when not aiming).
    void zoomSmooth(double targetZoom, double dt, double lerpSpeed = 6.0) {
        double t = 1.0 - std::exp(-lerpSpeed * dt);
        zoom_ += (targetZoom - zoom_) * t;
    }

    // Exponential smoothing toward a target — framerate-independent given
    // a per-frame dt. lerpSpeed higher = snappier follow.
    void followSmooth(double targetX, double targetY, double dt, double lerpSpeed = 8.0) {
        double t = 1.0 - std::exp(-lerpSpeed * dt);
        x_ += (targetX - x_) * t;
        y_ += (targetY - y_) * t;
    }

    // Same smoothing as followSmooth, but biased toward (targetX +
    // offsetX, targetY + offsetY) instead of straight at (targetX,
    // targetY) — a small "camera leans toward where you're aiming"
    // technique. Camera doesn't know what "aim" means; offsetX/offsetY
    // is just an extra vector the caller supplies (typically aimDir *
    // some max-pixels constant, smoothed at its own rate before being
    // passed in — see main.cpp) so this stays fully generic. A lower
    // lerpSpeed than the movement-follow call is usually what you want
    // here, so quick aim flicks don't visibly jitter the camera.
    void followSmoothWithOffset(double targetX, double targetY, double offsetX, double offsetY,
                                 double dt, double lerpSpeed = 8.0) {
        followSmooth(targetX + offsetX, targetY + offsetY, dt, lerpSpeed);
    }

    void worldToScreen(double wx, double wy, double& sx, double& sy) const {
        sx = (wx - x_) * zoom_ + viewportWidth_ * 0.5 + shakeX_;
        sy = (wy - y_) * zoom_ + viewportHeight_ * 0.5 + shakeY_;
    }

    // Inverse of worldToScreen — deliberately ignores shake offset, so
    // mouse-aim math stays stable even while the screen is shaking from
    // a hit; only the visual draw position should jitter, not where
    // the game thinks the cursor is pointing.
    void screenToWorld(double sx, double sy, double& wx, double& wy) const {
        wx = (sx - viewportWidth_ * 0.5) / zoom_ + x_;
        wy = (sy - viewportHeight_ * 0.5) / zoom_ + y_;
    }

    double zoom() const { return zoom_; }
    double x() const { return x_; }
    double y() const { return y_; }

    void setViewport(int w, int h) { viewportWidth_ = w; viewportHeight_ = h; }

    // Additive screen-space jitter for hit-impact feedback. Set once per
    // frame — typically from a short decaying timer in game code — and
    // it folds into every worldToScreen() call until overwritten or
    // reset to {0, 0}. Generic enough to belong in engine/: any top-down
    // game wants "shake the camera briefly," not just this one.
    void setShakeOffset(double x, double y) { shakeX_ = x; shakeY_ = y; }

private:
    double x_ = 0.0;
    double y_ = 0.0;
    double zoom_ = 1.0;
    double shakeX_ = 0.0;
    double shakeY_ = 0.0;
    int viewportWidth_;
    int viewportHeight_;
};

} // namespace engine::render
