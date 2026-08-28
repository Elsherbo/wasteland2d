#pragma once

#include <cmath>

#include "ecs/Attachment.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"

namespace engine::ecs {

// Run once per fixed update, after PhysicsSyncSystem has already
// written this tick's Transform for any parent with a RigidBody (see
// main.cpp's update-callback ordering) — otherwise attachments would
// trail one tick behind their parent.
class AttachmentSystem {
public:
    static void update(Registry& registry) {
        for (Entity e : registry.view<Attachment, Transform>()) {
            const auto& attachment = registry.get<Attachment>(e);

            // Parent destroyed (or never had a Transform) — leave this
            // entity wherever it last was rather than crash. A future
            // milestone might want "destroy attachments when their
            // parent dies" as an opt-in policy; that's a game-level
            // decision, not something this generic primitive assumes.
            if (!registry.isAlive(attachment.parent) || !registry.has<Transform>(attachment.parent)) {
                continue;
            }

            const auto& parentTransform = registry.get<Transform>(attachment.parent);
            auto& transform = registry.get<Transform>(e);

            if (attachment.followRotation) {
                double radians = parentTransform.rotationDegrees * (3.14159265358979323846 / 180.0);
                double c = std::cos(radians);
                double s = std::sin(radians);
                transform.x = parentTransform.x + (attachment.offsetX * c - attachment.offsetY * s);
                transform.y = parentTransform.y + (attachment.offsetX * s + attachment.offsetY * c);
                transform.rotationDegrees = parentTransform.rotationDegrees;
            } else {
                // Position still follows the parent; rotation is
                // deliberately left untouched — whatever system owns
                // this entity's aim/facing (e.g. a held weapon tracking
                // the mouse in main.cpp) is responsible for it, and
                // AttachmentSystem must never fight that system for
                // control of the same field.
                transform.x = parentTransform.x + attachment.offsetX;
                transform.y = parentTransform.y + attachment.offsetY;
            }
        }
    }
};

} // namespace engine::ecs
