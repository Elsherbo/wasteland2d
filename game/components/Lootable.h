#pragma once

#include <string>

namespace game::components {

// Marks an entity's Inventory as openable by another entity, via the
// Milestone 4 trigger/Interactable system. The presence of Inventory +
// Lootable together on the same entity is the actual condition; this
// component itself carries just enough data to build a sensible
// prompt. A locked container, if that's ever needed, would add a field
// here rather than needing a whole new component.
struct Lootable {
    std::string label = "container"; // used in the Interactable prompt, e.g. "Press E to loot <label>"
};

} // namespace game::components
