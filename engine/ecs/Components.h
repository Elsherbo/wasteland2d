#pragma once

namespace engine::ecs {

// World-space position/rotation/scale. Lives in engine/ because every
// future game built on this framework needs it — it has nothing
// Ashworld/Zero-Sievert-specific about it.
struct Transform {
    double x = 0.0;
    double y = 0.0;
    double rotationDegrees = 0.0;
    double scale = 1.0;
};

} // namespace engine::ecs
