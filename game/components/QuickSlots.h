#pragma once

#include <array>
#include <string>

namespace game::components {

// Hotbar bindings — number keys 4 through 9 (Primary/Secondary/Melee
// already own 1-3, see EquipmentSlots). A quick slot binds to an
// itemId, not a specific grid stack — consumables split and merge
// across stacks as they're picked up/used, so the binding has to
// survive that; UseItemSystem finds whichever stack currently has that
// itemId when the slot is actually used.
struct QuickSlots {
    static constexpr int kSlotCount = 6; // keys 4, 5, 6, 7, 8, 9

    // Empty string = unassigned. Index 0 = key 4, index 5 = key 9.
    std::array<std::string, kSlotCount> itemIds{};
};

} // namespace game::components
