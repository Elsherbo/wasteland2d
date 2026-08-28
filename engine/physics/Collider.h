#pragma once

namespace engine::physics {

// Collision box size, centered on the entity's Transform position — kept
// separate from render::Sprite because a sprite's visual size and its
// actual collision footprint are often different (e.g. a tall sprite
// with a small footprint at its base).
struct Collider {
    double width = 24.0;
    double height = 24.0;
};

} // namespace engine::physics
