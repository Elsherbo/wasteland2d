# wasteland2d — Milestone 6, data layer

Verified against the **real, unmodified `CMakeLists.txt`** this time (with one
addition — `nlohmann_json`), not just an ad-hoc compile script: I built
Box2D 3.1.1 from source, `cmake --install`-ed it to a prefix so
`find_package(box2d CONFIG)` resolves it exactly the way vcpkg would on
Windows, and ran the project's actual CMake configure + Ninja build end to
end. It configured, built, and ran headlessly without a single manual
workaround beyond that one dependency addition. Three standalone tests
(23 assertions total), all passing.

## Scope: this is the data layer, not the UI

Milestone 6 as scoped in ROADMAP.md includes a full drag-and-drop
inventory UI (`engine::ui`, panels, grid slots, hover/tooltips). **That's
not in this delivery.** What's here: the item database, the shaped-grid
`Inventory` component and all its placement/stacking/weight logic,
`Lootable`/`LootDrop`, and corpse-spawning on death — the entire *system*
a UI would sit on top of, fully working and tested. Looting itself
(pressing E near a corpse) works right now, just through a console
message instead of a drag-and-drop panel.

I split it this way on purpose: a real immediate-mode UI widget layer
(hit-testing, drag state, panel rendering) is substantial, separate
engineering, and cramming it in alongside the data layer in one pass
risked rushing the part that's actually hardest to get right after the
fact — grid placement, overlap, and weight-limit math, exactly the kind
of logic where the floating-point bug below was hiding. Get the model
correct and tested first; the view is next. Say if you'd rather I'd
built both together — this was a judgment call, not a constraint.

## What to do with this

```
CMakeLists.txt                       (replaces yours — one addition: nlohmann_json)
game/main.cpp                        (replaces yours)
game/systems/CombatSystem.h          (replaces yours)
game/systems/InventorySystem.h       (new)
game/components/Inventory.h          (new)
game/components/Lootable.h           (new)
game/components/LootDrop.h           (new)
game/data/ItemDefinition.h           (new)
game/data/ItemDatabase.h             (new)
game/data/ItemDatabase.cpp           (new)
engine/physics/PhysicsWorld.h        (replaces yours)
engine/physics/PhysicsWorld.cpp      (replaces yours)
assets/items/items.json              (new)
```

`tests/*.cpp` are standalone, no-SDL(-mostly) tests — not part of the game
build. `combat_test.cpp` needs Box2D (same as before); `inventory_test.cpp`
needs nothing but `nlohmann_json`; `attachment_test.cpp` (from Milestone
5.5, included for completeness) needs nothing at all.

**New dependency: `nlohmann_json`.** Your original `CMakeLists.txt`
comment already listed it in the `vcpkg install` line — it just wasn't
actually used until now. On Windows: `vcpkg install nlohmann-json:x64-windows`
if you haven't already. On Linux: `apt install nlohmann-json3-dev`. Unlike
SDL2/tinyxml2/box2d, it ships a proper CMake config on both platforms, so
no pkg-config fallback was needed in `CMakeLists.txt` — just one
`find_package(nlohmann_json CONFIG REQUIRED)` and one library on
`target_link_libraries`.

**If you're on the explicit-source-list CMake fix from the original
audit** (not the default `GLOB_RECURSE`): add `game/data/ItemDatabase.cpp`
to `GAME_SOURCES` by hand — I verified this delivery against the
*original* glob-based `CMakeLists.txt`, which picks it up automatically;
the explicit-list version won't.

---

## What's in it

- **`game/data/ItemDefinition.h` / `ItemDatabase.h/.cpp`** — item
  definitions loaded from `assets/items/items.json` (5 starter items:
  bandage, 9mm ammo, canned food, pistol, backpack). `nlohmann::json` is
  isolated to `ItemDatabase.cpp` — same pattern `PhysicsWorld` uses to
  hide Box2D and `TileMap` uses to hide `tinyxml2`, so nothing that just
  *looks up* an item pulls in the JSON parser's header.

- **`game/components/Inventory.h` + `game/systems/InventorySystem.h`** —
  the shaped grid you confirmed (items occupy a `width x height`
  footprint, not one item per slot — Tarkov/Zero-Sievert-style, not
  simple stacking). `InventorySystem` owns placement (`canPlace`,
  `findFreePosition`), stacking + weight-limited `addItem`, and
  `moveAllTo` for transferring one inventory into another — respecting
  both grid space and weight, leaving whatever doesn't fit behind rather
  than an all-or-nothing transfer. This is genuinely the most
  logic-dense code in the delivery, and it shows in the test count.

- **`game/components/Lootable.h` / `LootDrop.h`** — `Lootable` marks an
  entity's `Inventory` as openable (used by the existing Milestone 4
  Interactable/trigger system — no new interaction machinery needed).
  `LootDrop` is attached to whatever should spawn a corpse on death; the
  target dummy has one (2 bandages, 30 9mm rounds, 1 pistol). An entity
  with `Health` but no `LootDrop` still just despawns, exactly like
  Milestone 5.

- **`CombatSystem`'s death handling** — now spawns a corpse (walkable,
  not solid — see the `PhysicsWorld` change below — with an `Inventory`
  populated from `LootDrop`, an `Interactable` prompt, and a sensor body
  for interaction range) instead of despawning outright, for any dying
  entity that has a `LootDrop`. This was literally flagged as a TODO in
  the Milestone 5 delivery's own comment — replacing exactly that branch.

- **`PhysicsWorld::BodyParams::solid`** — small, additive change: `false`
  skips creating a body's solid box fixture entirely, so
  `addCircleSensor` can still attach an interaction-range sensor to a
  body that doesn't block movement. Defaults to `true`, so every
  existing call site is completely unaffected — this is what makes a
  corpse walkable instead of an invisible wall where the target dummy
  used to stand.

- **Looting in `main.cpp`** — pressing E while `InteractionTracker`
  reports a nearby `Lootable` + `Inventory` entity calls
  `InventorySystem::moveAllTo` into the player's own `Inventory` and
  prints what happened. This is the stand-in for the real UI mentioned
  above.

---

## A real bug I found and fixed while testing this

`InventorySystem::addItem`'s weight-budget check —
`static_cast<int>(budget / weight)` — looked correct and matched the math
I'd worked out by hand. The `inventory_test.cpp` `moveAllTo` case caught
it anyway: a 0.7kg weight budget with 0.1kg-per-unit items should fit
exactly 7 units, but the test kept coming up with 6.

```
>>> 0.7 - 0.5
0.19999999999999996
>>> (0.7 - 0.5) / 0.1
1.9999999999999996
>>> int((0.7 - 0.5) / 0.1)
1          # should be 2
```

Decimal weights like `0.1`/`0.7` essentially never have an exact binary
floating-point representation, so a budget that's *mathematically* exactly
enough for N units routinely evaluates to something like `N - 0.0000000004`
and a plain truncating cast rounds that down to `N - 1`. In a
weight-limited inventory, that's a real, player-visible bug: "your bag is
full" when there's genuinely exactly enough room. Fixed with the standard
mitigation — a small epsilon before truncating
(`budget / weight + 1e-4f`) — and the `moveAllTo` test (along with a
separate, simpler weight-limit test) now both pass with exact expected
values, not just "close enough."

I also want to flag one thing that *wasn't* a bug, but did cost debugging
time: my first version of the grid-packing test asserted a third 2x2
crate couldn't fit in a 4x4 grid. It's wrong — a 4x4 grid fits exactly
four 2x2 crates, one per quadrant, and `InventorySystem` correctly finds
the third and fourth placements. That was my test's arithmetic being
wrong, not the code; worth knowing in case you extend these tests later
and want to sanity-check grid math by hand first.

---

## How I verified this

1. **Built the real `CMakeLists.txt`** (not an ad-hoc script) against a
   from-source Box2D 3.1.1 installed to a proper CMake package prefix —
   the closest I can get in this environment to exactly reproducing your
   vcpkg-based Windows build. Configured and built cleanly with Ninja,
   zero manual workarounds beyond the one `nlohmann_json` addition this
   delivery itself makes.
2. **Ran the CMake-built binary headlessly** (Xvfb) — confirms the full
   entity setup (item database load, player `Inventory`, dummy
   `LootDrop`, corpse-spawn wiring) initializes and the main loop runs
   without crashing, and that the asset-copy step correctly picked up
   `assets/items/items.json`.
3. **`inventory_test.cpp`** (8 checks) — stacking math, shaped-item
   overlap (including full 4-quadrant grid packing), grid-full rejection,
   weight-limit enforcement, unknown-item-id handling, and the
   `moveAllTo` partial-transfer/weight-exhaustion scenario that caught
   the bug above.
4. **`combat_test.cpp`** (12 checks, expanded from Milestone 5) — the
   original hitscan/cooldown/damage/despawn checks unchanged as a
   regression test, plus new checks that killing a `LootDrop`-bearing
   entity spawns exactly one corpse, at the correct death position, with
   the correct items, with the right components (`Interactable`,
   `RigidBody`) — and that looting it via `moveAllTo` actually works.
5. **`attachment_test.cpp`** rerun unchanged — confirms nothing about
   Milestone 5.5's attachment mechanism regressed.

I still don't have a display — pressing E in-game, watching the console
output, and seeing the corpse render (dim gray rectangle, walkable) is
worth you confirming directly once you've built it.
