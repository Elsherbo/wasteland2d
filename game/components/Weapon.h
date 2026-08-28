#pragma once

namespace game::components {

// A single hitscan-capable weapon. Projectile weapons (travel-time,
// ballistic drop) are meant to become a separate Weapon *type* on this
// same component later (see ROADMAP.md Milestone 5) — not a rewrite of
// this one, so keep new fields additive rather than replacing these.
struct Weapon {
    float fireRate = 6.0f;        // shots per second
    float damage = 25.0f;
    float range = 800.0f;         // pixels
    float spreadDegrees = 2.0f;   // max random deviation per shot, either side of aim

    // Aim-down-sights (right-click held): how far the camera leans
    // toward the aim direction, and how much it zooms in, while ADS is
    // active — see main.cpp's ADS handling. Deliberately per-weapon
    // rather than one global constant: a pistol should barely zoom, an
    // AR more, a sniper significantly more — this is the field a real
    // AR/sniper Weapon instance would set differently once Milestone
    // 6's equipment slots let more than one weapon exist at a time.
    // Defaults here are pistol-ish (mild zoom, modest lean).
    float adsZoomMultiplier = 1.15f;  // camera zoom while aiming; 1.0 = no zoom
    float adsOffsetPixels = 50.0f;    // camera lean toward aim direction while aiming, in world pixels

    // Seconds remaining before the next shot is allowed. CombatSystem
    // owns this entirely — nothing else should write to it.
    float cooldown = 0.0f;
};

} // namespace game::components
