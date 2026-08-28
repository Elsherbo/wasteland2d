#pragma once

#include <cstdint>

namespace engine::ecs {

// An Entity is just a packed integer: low 20 bits are the slot index,
// high 12 bits are a generation counter. When a slot is destroyed and
// reused, its generation increments — so an old Entity handle someone
// is still holding (e.g. a bullet's "owner" reference after the owner
// died) will fail isAlive() instead of silently pointing at a new,
// unrelated entity that happens to reuse the same slot. This matters
// once things are spawning/dying constantly (bullets, corpses, loot).
using Entity = std::uint32_t;

constexpr std::uint32_t kEntityIndexBits = 20;
constexpr std::uint32_t kEntityIndexMask = (1u << kEntityIndexBits) - 1u;
constexpr Entity kNullEntity = 0xFFFFFFFFu;

constexpr std::uint32_t entityIndex(Entity e) { return e & kEntityIndexMask; }
constexpr std::uint32_t entityGeneration(Entity e) { return e >> kEntityIndexBits; }
constexpr Entity makeEntity(std::uint32_t index, std::uint32_t generation) {
    return (generation << kEntityIndexBits) | (index & kEntityIndexMask);
}

} // namespace engine::ecs
