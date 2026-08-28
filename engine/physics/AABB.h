#pragma once

namespace engine::physics {

// Top-left-corner rect in world pixel space — matches how Tiled reports
// object-layer rectangles, so collision data loaded from a .tmx needs no
// coordinate conversion to land here.
struct AABB {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
};

inline bool intersects(const AABB& a, const AABB& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

// Build an AABB centered on (cx, cy) — convenient since Transform/Sprite
// work in center-of-entity coordinates but AABB is corner-based.
inline AABB aabbFromCenter(double cx, double cy, double w, double h) {
    return AABB{cx - w * 0.5, cy - h * 0.5, w, h};
}

} // namespace engine::physics
