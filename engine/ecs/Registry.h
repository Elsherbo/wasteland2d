#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ecs/ComponentPool.h"
#include "ecs/Entity.h"

namespace engine::ecs {

// Owns entity lifecycle (create/destroy, generation counters) and every
// component pool. This is the whole ECS API — game code creates
// entities, attaches components, and iterates views; it never touches
// ComponentPool directly.
class Registry {
public:
    Entity create() {
        std::uint32_t idx;
        if (!freeIndices_.empty()) {
            idx = freeIndices_.back();
            freeIndices_.pop_back();
        } else {
            idx = static_cast<std::uint32_t>(generations_.size());
            generations_.push_back(0);
        }
        return makeEntity(idx, generations_[idx]);
    }

    void destroy(Entity e) {
        if (!isAlive(e)) return;
        std::uint32_t idx = entityIndex(e);

        for (auto& [type, pool] : pools_) {
            pool->remove(e);
        }

        generations_[idx] = (generations_[idx] + 1) & ((1u << (32 - kEntityIndexBits)) - 1u);
        freeIndices_.push_back(idx);
    }

    bool isAlive(Entity e) const {
        std::uint32_t idx = entityIndex(e);
        return idx < generations_.size() && generations_[idx] == entityGeneration(e);
    }

    template <typename T, typename... Args>
    T& emplace(Entity e, Args&&... args) {
        return poolFor<T>().emplace(e, std::forward<Args>(args)...);
    }

    // WARNING — reference/pointer stability: emplace<T>() and remove<T>()
    // can invalidate EVERY existing T& or T* obtained from get<T>(),
    // for ANY entity, not just the one being added/removed — not a
    // hypothetical edge case, a real bug this project shipped and had
    // to debug (see game/main.cpp's history: a long-lived `auto&
    // playerInventory = registry.get<Inventory>(player);` silently
    // went stale the first time ANY corpse spawned with its own
    // Inventory component, because ComponentPool<T>'s dense storage is
    // a std::vector<T> — emplace_back can reallocate it, and remove()
    // does swap-and-pop, which can silently redirect an existing
    // reference to a DIFFERENT entity's data even with no
    // reallocation at all). This mirrors the explicit, well-known
    // contract real production ECS libraries (EnTT, etc.) document for
    // exactly this reason.
    //
    // The rule: never hold a T& (or T*) across ANY emplace<T>()/
    // remove<T>() call for the same T, for ANY entity — including
    // calls you don't immediately see, buried inside a function you're
    // calling. Re-fetch via get<T>(e) immediately before every use
    // instead of caching the reference in a variable that outlives a
    // single, uninterrupted read. This is exactly why callers
    // throughout this codebase re-fetch via registry.get<T>(entity)
    // right before use rather than storing it in a long-lived local —
    // follow that pattern, not the one that broke it.
    template <typename T>
    void remove(Entity e) {
        poolFor<T>().remove(e);
    }

    template <typename T>
    bool has(Entity e) const {
        auto it = pools_.find(std::type_index(typeid(T)));
        return it != pools_.end() && it->second->has(e);
    }

    // See the WARNING above remove() — the returned reference is only
    // valid until the next emplace<T>()/remove<T>() call for this same
    // T (for any entity). Do not store it in a variable that outlives
    // that.
    template <typename T>
    T& get(Entity e) {
        return poolFor<T>().get(e);
    }

    template <typename T>
    const T& get(Entity e) const {
        return poolFor<T>().get(e);
    }

    // Returns entities that have every component type in Ts...
    // Naive but simple: drives iteration off whichever pool among Ts
    // has the fewest entities, then filters against the rest. Fine
    // until profiling says otherwise — premature optimization here
    // (e.g. archetype tables) would cost more in complexity than it's
    // worth at this project's scale.
    template <typename... Ts>
    std::vector<Entity> view() {
        static_assert(sizeof...(Ts) >= 1, "view<>() needs at least one component type");
        std::vector<Entity> result;

        std::size_t smallest = std::numeric_limits<std::size_t>::max();
        const std::vector<Entity>* driver = nullptr;
        (void)std::initializer_list<int>{
            (driver = poolSizeOf<Ts>() < smallest
                          ? (smallest = poolSizeOf<Ts>(), &poolFor<Ts>().entities())
                          : driver,
             0)...};

        if (!driver) return result;

        result.reserve(driver->size());
        for (Entity e : *driver) {
            if ((has<Ts>(e) && ...)) result.push_back(e);
        }
        return result;
    }

private:
    template <typename T>
    ComponentPool<T>& poolFor() {
        std::type_index key(typeid(T));
        auto it = pools_.find(key);
        if (it == pools_.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            ComponentPool<T>& ref = *pool;
            pools_.emplace(key, std::move(pool));
            return ref;
        }
        return static_cast<ComponentPool<T>&>(*it->second);
    }

    template <typename T>
    std::size_t poolSizeOf() {
        return poolFor<T>().size();
    }

    std::vector<std::uint32_t> generations_;
    std::vector<std::uint32_t> freeIndices_;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools_;
};

} // namespace engine::ecs
