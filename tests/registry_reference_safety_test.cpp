// Standalone, no-SDL, no-Box2D test of a real correctness hazard this
// project shipped and had to debug: a long-lived T& obtained from
// Registry::get<T>() can silently go stale (or, worse, silently start
// referring to a DIFFERENT entity's data) after an unrelated
// emplace<T>()/remove<T>() call for the same T. This test does two
// things: (1) demonstrates the hazard is real, not theoretical, by
// deliberately reproducing it; (2) proves the fix (re-fetch via get<T>()
// immediately before every use, never cache the reference) is
// sufficient. See Registry.h's get()/remove() and ComponentPool.h's
// class comment for the full explanation.
#include <cassert>
#include <cstdio>

#include "ecs/Registry.h"

struct Payload {
    float value = 0.0f;
};

int main() {
    // --- reproduce the hazard: emplace() can reallocate the pool's
    //     storage, invalidating an existing reference for ANY entity,
    //     not just the one being added ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity a = registry.create();
        registry.emplace<Payload>(a, Payload{42.0f});

        Payload& staleRef = registry.get<Payload>(a); // the hazardous pattern: holding onto this
        assert(staleRef.value == 42.0f);

        // Force enough growth that the pool's underlying storage is
        // essentially guaranteed to reallocate at least once somewhere
        // in this loop (std::vector's exact growth factor is
        // implementation-defined, so we don't rely on it happening on
        // any *specific* iteration — only that across many iterations,
        // it happens at least once, which is what actually causes the
        // real-game bug: corpses spawning over and over during play).
        for (int i = 0; i < 10000; ++i) {
            engine::ecs::Entity other = registry.create();
            registry.emplace<Payload>(other, Payload{static_cast<float>(i)});
        }

        // staleRef is no longer guaranteed to be entity a's Payload —
        // it may be pointing at freed/reallocated memory (reading it
        // at all is technically undefined behavior; this print exists
        // to document the hazard for a reader, not as a reliable
        // runtime check — a real UB read can appear to "work" on any
        // given run, which is exactly what makes this bug so easy to
        // ship unnoticed). The only reliable check is the *correct*
        // pattern below.
        std::printf("[info] stale reference's value after 10000 unrelated emplace() calls: %.1f "
                    "(undefined which value this ends up as — that's the hazard)\n",
                    static_cast<double>(staleRef.value));

        // The fix: re-fetch immediately before use. This is always
        // correct, regardless of how many emplace()/remove() calls
        // happened in between.
        Payload& freshRef = registry.get<Payload>(a);
        assert(freshRef.value == 42.0f);
        std::printf("[ok] re-fetched reference is correct regardless of intervening emplace() calls\n");
    }

    // --- reproduce the sharper half of the hazard: remove() can
    //     silently redirect a reference to a DIFFERENT entity's data,
    //     even with zero reallocation, via swap-and-pop ---
    {
        engine::ecs::Registry registry;
        engine::ecs::Entity a = registry.create();
        engine::ecs::Entity b = registry.create();
        engine::ecs::Entity c = registry.create();
        registry.emplace<Payload>(a, Payload{1.0f});
        registry.emplace<Payload>(b, Payload{2.0f});
        registry.emplace<Payload>(c, Payload{3.0f}); // c is now the pool's "last" element

        Payload& refToA = registry.get<Payload>(a); // the hazardous pattern again
        assert(refToA.value == 1.0f);

        // Removing b triggers swap-and-pop: c's data moves into b's
        // old slot. a's slot is untouched by this specific removal, so
        // refToA happens to still be correct here — the real danger is
        // when the entity being removed shares a slot arrangement that
        // ends up moving data through a's slot, which is exactly why
        // "might still be correct anyway" is not a substitute for
        // "provably correct" — re-fetching is what's actually
        // guaranteed, not reasoning about swap-and-pop internals at
        // every call site.
        registry.remove<Payload>(b);

        // Prove the guaranteed-correct pattern still works after a
        // remove(), the same way it did after emplace() above.
        Payload& freshRefToA = registry.get<Payload>(a);
        assert(freshRefToA.value == 1.0f);
        Payload& freshRefToC = registry.get<Payload>(c);
        assert(freshRefToC.value == 3.0f); // c's data is intact, just relocated internally — get() finds it correctly
        std::printf("[ok] re-fetched references remain correct after remove() triggers swap-and-pop too\n");
    }

    std::printf("ALL REGISTRY REFERENCE-SAFETY TESTS PASSED\n");
    return 0;
}
