# wasteland2d — two real bugs, both fixed

Both of your questions pointed at real, confirmed problems, not user
error. Here's exactly what was wrong and what changed.

## Bug 1: "why did I get slower after killing the dummy?"

**Root cause: a dangling reference.** `main.cpp` captured
`auto& playerInventory = registry.get<Inventory>(player);` once at
startup and kept using that same reference for the entire game
session. `ComponentPool<T>` stores components in a `std::vector<T>` —
`emplace()` can reallocate that vector (invalidating *every* existing
reference into it, for *every* entity, not just the one being added),
and `remove()`'s swap-and-pop can silently redirect a reference to a
completely different entity's data even with zero reallocation.

Every corpse spawn does `registry.emplace<Inventory>(corpse, ...)` —
so the moment you killed anything with loot, that emplace call could
invalidate `playerInventory`. From then on, `Encumbrance::
speedMultiplier()` was reading garbage memory as your carry weight,
which is exactly why it looked like killing something (not picking
anything up) made you slower — nothing about your actual inventory
changed, the *reference to it* just went bad.

**The fix, two parts:**
1. Every place `main.cpp` used that long-lived reference now re-fetches
   fresh via `registry.get<Inventory>(player)` immediately before use
   instead. The setup-time version is now explicitly scoped to a block
   that runs entirely before the game loop starts (where it's provably
   safe), with a comment explaining why it must never be extended past
   that point again.
2. **The actual root-cause fix**: `Registry::get()`/`remove()` and
   `ComponentPool`'s class comment now carry an explicit, loud warning
   about this hazard — the same contract real ECS libraries (EnTT,
   etc.) document for exactly this reason. This isn't just a comment
   for its own sake: it's there so this exact mistake doesn't get
   quietly reintroduced somewhere else later, by me or by you.

**Verified with a new `registry_reference_safety_test.cpp`** that
deliberately reproduces the hazard — the "stale" print in its output is
a real garbage float value pulled from reused/reallocated memory (you
can see the actual UB in the test's own output, not just an assertion
that it "would" happen) — and proves the re-fetch pattern is immune to
it. Plus a second test, `kill_scenario_test.cpp`, that reproduces your
*exact* reported scenario end to end: arm a player, kill a
loot-bearing target (spawning a corpse, growing the `Inventory` pool,
the precise trigger), and confirm encumbrance still reads correctly
afterward.

## Bug 2: "why can't I add/remove quick slots or weapons?"

**Adding worked. Removing never existed at all** — I built and
thoroughly tested `EquipmentSystem::unequip()` back when this system
was first delivered, but never actually wired it to any key in
`main.cpp`. Same gap for clearing a hotbar slot. You weren't doing
anything wrong; the feature was genuinely half-built.

**Fixed:** one key now does both directions, decided by what's under
your cursor:
- **1/2/3** while hovering a compatible weapon in your inventory grid:
  equips it. The same key while hovering **nothing** (empty space, not
  an invalid item): unequips that slot back to your grid.
- **B**: same pattern, for the backpack slot.
- **Ctrl+4-9**: hovering a usable item binds it to that hotbar slot;
  hovering nothing clears it.

I also added console feedback for all of this (`[equip] equipped`,
`[equip] unequipped`, `[equip] switched to secondary`,
`[quickslot] bound key 4`, etc.) — there was previously zero
confirmation that any of these actions succeeded, which likely made it
*look* broken even on the parts that technically worked.

**One deliberate design choice worth knowing:** if you press 1/2/3
while hovering an item that's *not* equippable there (like a bandage
over the Primary slot), that correctly refuses to equip — and does
*not* fall back to unequipping. Only truly empty space triggers
unequip. This avoids an accidental unequip when you meant to equip
something invalid.

## What to do with this

```
game/main.cpp              (replaces yours)
engine/ecs/Registry.h      (replaces yours — adds the safety warning)
engine/ecs/ComponentPool.h (replaces yours — adds the safety warning)
```

`tests/registry_reference_safety_test.cpp` and
`tests/kill_scenario_test.cpp` are new; the rest are included unchanged
for a self-contained package. No `CMakeLists.txt` change needed.

## How I verified this

1. New `registry_reference_safety_test.cpp` (3 checks) — reproduces
   both halves of the hazard (reallocation via `emplace()`, silent
   redirection via `remove()`'s swap-and-pop) and proves the re-fetch
   pattern is correct against both.
2. New `kill_scenario_test.cpp` (3 checks) — your exact scenario, end
   to end: confirmed correct before any kill, confirmed still correct
   after killing a loot-bearing target spawns a corpse.
3. Reran all eight existing standalone suites unchanged — all still
   pass, confirming the fix didn't touch any other behavior.
4. Real `CMakeLists.txt` build (Ninja), headless run of the actual
   built binary, zero crashes.

Try it and confirm the slowdown is actually gone after a kill this
time, and that 1/2/3/B/Ctrl+4-9 now let you both equip and unequip.
