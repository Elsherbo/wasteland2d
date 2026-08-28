#pragma once

#include <unordered_set>
#include <vector>

#include "ecs/Entity.h"
#include "physics/PhysicsWorld.h"

namespace engine::physics {

// Feed it every frame's drainTriggerEvents() output; it maintains the
// set of entities currently overlapping `watcher` (typically the
// player). Generic on purpose — any top-down game with "press E"
// prompts wants "what am I near right now," derived once here instead
// of every system that cares re-deriving it from raw trigger events.
class InteractionTracker {
public:
    void update(ecs::Entity watcher, const std::vector<TriggerEvent>& events) {
        for (const TriggerEvent& e : events) {
            ecs::Entity other = ecs::kNullEntity;
            if (e.a == watcher) other = e.b;
            else if (e.b == watcher) other = e.a;
            else continue; // this trigger doesn't involve the watched entity at all

            if (e.began) nearby_.insert(other);
            else nearby_.erase(other);
        }
    }

    const std::unordered_set<ecs::Entity>& nearby() const { return nearby_; }

private:
    std::unordered_set<ecs::Entity> nearby_;
};

} // namespace engine::physics
