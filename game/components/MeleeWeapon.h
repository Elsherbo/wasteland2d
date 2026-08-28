#pragma once

namespace game::components {

// A melee weapon's stats — deliberately a separate component from
// Weapon (ranged/hitscan), not a variant of it. See
// MeleeCombatSystem's own header comment for why melee is a genuinely
// different mechanic (a short-range arc hit-check, not a raycast), not
// a reduced-range hitscan weapon.
struct MeleeWeapon {
    float damage = 30.0f;
    float range = 55.0f;          // pixels, swing reach from the attacker's origin
    float arcDegrees = 90.0f;     // total width of the swing arc, centered on the aim direction
    float attacksPerSecond = 2.0f;

    // Seconds remaining before the next swing is allowed — mirrors
    // Weapon::cooldown exactly. MeleeCombatSystem owns this entirely;
    // nothing else should write to it.
    float cooldown = 0.0f;
};

} // namespace game::components
