// Standalone, dependency-free test of AttachmentSystem: no SDL, no
// Box2D — Transform/Attachment/Registry don't need either. Same style
// as the Milestone 2/3 headless tests the README references.
#include <cassert>
#include <cmath>
#include <cstdio>

#include "ecs/Attachment.h"
#include "ecs/AttachmentSystem.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"

namespace {
bool nearlyEqual(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }
} // namespace

int main() {
    engine::ecs::Registry registry;

    engine::ecs::Entity parent = registry.create();
    registry.emplace<engine::ecs::Transform>(parent, 100.0, 50.0, 30.0); // x, y, rotationDegrees

    // followRotation = true: offset should rotate with the parent, and
    // rotationDegrees should be copied from it.
    engine::ecs::Entity rotatingChild = registry.create();
    registry.emplace<engine::ecs::Transform>(rotatingChild, 0.0, 0.0, 0.0);
    registry.emplace<engine::ecs::Attachment>(rotatingChild, engine::ecs::Attachment{parent, 10.0f, 0.0f, true});

    // followRotation = false: position still follows, but rotation is
    // left completely alone — set to a sentinel value to confirm
    // AttachmentSystem never touches it.
    engine::ecs::Entity fixedRotationChild = registry.create();
    registry.emplace<engine::ecs::Transform>(fixedRotationChild, 0.0, 0.0, 77.0);
    registry.emplace<engine::ecs::Attachment>(fixedRotationChild,
                                               engine::ecs::Attachment{parent, 5.0f, 5.0f, false});

    engine::ecs::AttachmentSystem::update(registry);

    // Expected: parent at (100, 50), rotated 30°. offset (10, 0)
    // rotated by 30° = (10*cos30, 10*sin30) = (8.660254, 5.0).
    const auto& rotated = registry.get<engine::ecs::Transform>(rotatingChild);
    assert(nearlyEqual(rotated.x, 100.0 + 10.0 * std::cos(30.0 * M_PI / 180.0)));
    assert(nearlyEqual(rotated.y, 50.0 + 10.0 * std::sin(30.0 * M_PI / 180.0)));
    assert(nearlyEqual(rotated.rotationDegrees, 30.0));
    std::printf("[ok] followRotation=true: pos=(%.4f, %.4f) rot=%.2f\n", rotated.x, rotated.y,
                rotated.rotationDegrees);

    // Expected: raw offset added, no rotation applied to the offset,
    // and rotationDegrees left at its sentinel 77.0 — untouched.
    const auto& fixed = registry.get<engine::ecs::Transform>(fixedRotationChild);
    assert(nearlyEqual(fixed.x, 105.0));
    assert(nearlyEqual(fixed.y, 55.0));
    assert(nearlyEqual(fixed.rotationDegrees, 77.0));
    std::printf("[ok] followRotation=false: pos=(%.4f, %.4f) rot=%.2f (untouched)\n", fixed.x, fixed.y,
                fixed.rotationDegrees);

    // Destroy the parent, then run the system again — attached
    // entities must be left exactly where they were, not crash and not
    // silently move to (0,0) or similar.
    registry.destroy(parent);
    engine::ecs::AttachmentSystem::update(registry);

    const auto& rotatedAfterParentGone = registry.get<engine::ecs::Transform>(rotatingChild);
    assert(nearlyEqual(rotatedAfterParentGone.x, rotated.x));
    assert(nearlyEqual(rotatedAfterParentGone.y, rotated.y));
    std::printf("[ok] parent destroyed: attached entity's transform left unchanged, no crash\n");

    std::printf("ALL ATTACHMENTSYSTEM TESTS PASSED\n");
    return 0;
}
