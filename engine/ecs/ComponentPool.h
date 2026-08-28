#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "ecs/Entity.h"

namespace engine::ecs {

// Type-erased base so Registry can hold a heterogeneous collection of
// pools (one per component type) and still call remove()/has() on
// whichever ones actually contain a given entity, without knowing T.
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(Entity e) = 0;
    virtual bool has(Entity e) const = 0;
};

// Sparse set: sparse_[index(e)] -> position in dense_/components_, or
// kInvalid if absent. Iteration over components_ is fully packed and
// contiguous (good for cache behavior); has()/get()/remove() are O(1).
//
// Reference stability: components_ is a std::vector<T> — emplace()'s
// components_.emplace_back() can reallocate it, invalidating every
// existing T&/T* this pool has ever handed out, for every entity, not
// just the one being added. remove()'s swap-and-pop (below) is worse:
// components_[pos] = std::move(components_[lastPos]) can silently
// redirect an existing reference to a *different* entity's data with
// no reallocation at all. See Registry.h's get()/remove() for the
// resulting rule callers must follow — this is why that warning lives
// there, not just here: it's a Registry-level contract, not a detail
// of this one class.
template <typename T>
class ComponentPool : public IComponentPool {
public:
    static constexpr std::uint32_t kInvalid = std::numeric_limits<std::uint32_t>::max();

    template <typename... Args>
    T& emplace(Entity e, Args&&... args) {
        std::uint32_t idx = entityIndex(e);
        if (idx >= sparse_.size()) sparse_.resize(idx + 1, kInvalid);

        if (sparse_[idx] != kInvalid) {
            // Already present — replace in place rather than duplicate.
            components_[sparse_[idx]] = T(std::forward<Args>(args)...);
            return components_[sparse_[idx]];
        }

        sparse_[idx] = static_cast<std::uint32_t>(dense_.size());
        dense_.push_back(e);
        components_.emplace_back(std::forward<Args>(args)...);
        return components_.back();
    }

    void remove(Entity e) override {
        std::uint32_t idx = entityIndex(e);
        if (idx >= sparse_.size() || sparse_[idx] == kInvalid) return;

        std::uint32_t pos = sparse_[idx];
        std::uint32_t lastPos = static_cast<std::uint32_t>(dense_.size() - 1);

        // Swap-and-pop: move the last element into the removed slot so
        // dense_/components_ stay contiguous with no holes.
        dense_[pos] = dense_[lastPos];
        components_[pos] = std::move(components_[lastPos]);
        sparse_[entityIndex(dense_[pos])] = pos;

        dense_.pop_back();
        components_.pop_back();
        sparse_[idx] = kInvalid;
    }

    bool has(Entity e) const override {
        std::uint32_t idx = entityIndex(e);
        return idx < sparse_.size() && sparse_[idx] != kInvalid;
    }

    T& get(Entity e) { return components_[sparse_[entityIndex(e)]]; }
    const T& get(Entity e) const { return components_[sparse_[entityIndex(e)]]; }

    // Exposed for Registry::view() — the pool with the fewest entities
    // is used as the iteration driver, then filtered against the rest.
    const std::vector<Entity>& entities() const { return dense_; }
    std::size_t size() const { return dense_.size(); }

private:
    std::vector<std::uint32_t> sparse_;
    std::vector<Entity> dense_;
    std::vector<T> components_;
};

} // namespace engine::ecs
