#pragma once

#include <string>

namespace engine::physics {

// Attach to any entity that should show an interaction prompt when the
// player (or whichever entity is tracked via InteractionTracker) enters
// its sensor range — a lootable corpse (Milestone 6), a vehicle
// (Milestone 7), a door. This component only carries what to *say*;
// deciding when to say it is InteractionTracker's job, and deciding how
// to render it is a game/UI concern (Milestone 6's UI system).
struct Interactable {
    std::string promptText;
};

} // namespace engine::physics
