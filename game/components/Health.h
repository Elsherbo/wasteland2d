#pragma once

namespace game::components {

// Ashworld-specific damage/death state. Kept in game/, not engine/ —
// "dead" here just flips a flag; what actually happens on death
// (despawn now, corpse+loot in Milestone 6) is gameplay policy, not
// something a reusable framework should have an opinion on.
struct Health {
    float current = 100.0f;
    float max = 100.0f;
    bool dead = false;
};

// Clamped damage application — never lets current go negative, and
// dead is sticky (a dead entity can't be re-damaged into "more dead").
// Free function rather than a method so it stays a trivial POD struct,
// consistent with every other component in this codebase.
inline void applyDamage(Health& health, float amount) {
    if (health.dead || amount <= 0.0f) return;
    health.current -= amount;
    if (health.current <= 0.0f) {
        health.current = 0.0f;
        health.dead = true;
    }
}

} // namespace game::components
