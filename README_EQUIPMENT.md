# wasteland2d — equipment slots (primary / secondary / melee)

Closes the loop on the very first thing you asked for, back before
Milestone 5.5. Same bar as always: real `CMakeLists.txt` build, headless
run (including real startup with the full starter-loadout equip
pipeline running against the actual `items.json`), and now **six**
standalone test suites — two new this pass (`melee_test.cpp`,
`equipment_test.cpp`), 17 new assertions between them, everything else
rerun unchanged to confirm no regression.

## What to do with this

```
game/main.cpp                        (replaces yours)
engine/input/InputManager.h          (replaces yours)
engine/input/InputManager.cpp        (replaces yours)
game/components/EquipmentSlots.h     (new)
game/components/MeleeWeapon.h        (new)
game/data/ItemDefinition.h           (replaces yours)
game/data/ItemDatabase.cpp           (replaces yours)
game/systems/CombatSystem.h          (replaces yours — refactored, see below)
game/systems/Damage.h                (new)
game/systems/EquipmentSystem.h       (new)
game/systems/MeleeCombatSystem.h     (new)
assets/items/items.json              (replaces yours — adds SMG, combat knife, weapon stats)
```

No `CMakeLists.txt` changes needed — every new file is a header, and
`ItemDatabase.cpp` already exists as a tracked source.

## Controls

- **1 / 2 / 3** while the inventory is **open** and hovering a
  compatible weapon: equips it to Primary/Secondary/Melee. Only takes
  effect immediately if that slot happens to already be active.
- **1 / 2 / 3** while the inventory is **closed**: switches your
  in-hand weapon among what's equipped.
- **Scroll wheel** while the inventory is closed: cycles the same way.
- Left-click / Fire: shoots (ranged slots active) or swings (melee
  active) — same input as before, now routes to whichever mechanic is
  actually equipped.

Player starts with a pistol (primary), SMG (secondary), and combat
knife (melee) all pre-equipped — same "can fight from frame one" feel
earlier milestones had via a hardcoded `Weapon` component, now actually
flowing through the real equip pipeline.

## Two design decisions worth knowing about

1. **Melee is a genuinely separate mechanic, not a hitscan variant** —
   per your call. `MeleeCombatSystem::swing()` does a real arc hit-check
   (distance + angle against every `Health`-bearing entity, all in
   range and in the swing's arc get hit in one cleave — not just the
   closest). It's not a Box2D/physics query; at this game's current
   entity counts that's fine, and the header comment says exactly when
   it would stop being fine (hundreds of simultaneous targets).
2. **Equipping and switching are deliberately different actions** —
   assigning an item to a slot (number key + hover, inventory open)
   never changes what's in your hand; switching (number keys or
   scroll, inventory closed) never touches the inventory grid. This
   matches how most shooters actually separate loadout management from
   moment-to-moment weapon switching, and was the natural reading of
   your two separate answers (melee mechanic + switching input) — but
   it's a real interpretation, not the only possible one, so flagging
   it plainly.

## A refactor this needed, and why — same bug-prevention reasoning as before

The "apply damage, kill it, maybe spawn a corpse" logic used to live
only inside `CombatSystem::fireWeapon`. Adding a second combat mechanic
(melee) that needs the *exact* same logic meant a real choice: duplicate
it, or extract it. Duplicating it risked quietly reintroducing the
leaked-physics-body bug from the Milestone 5 delivery — that fix lived
in one specific spot, and a second combat path that doesn't know about
it is exactly how that class of bug comes back. Extracted into
`Damage::apply()` (`Damage.h`); both `CombatSystem` and
`MeleeCombatSystem` now call it, and it exists in exactly one place.
`combat_test.cpp` reran with zero changes and all 12 checks still pass,
confirming the refactor didn't alter ranged combat's behavior at all.

## A real bug caught before it ever compiled

While wiring the player's starter loadout, I wrote three near-identical
loops (`for (const auto& stack : playerInventory.stacks) { if
(stack.itemId == "pistol") { equip(...); } }`) — the first one didn't
`break` after the match. `equip()` erases from that exact vector
internally, and continuing a range-for over a `std::vector` after
something erased from it mid-iteration is undefined behavior in C++,
not just a style nitpick. Caught this by inspection before ever
compiling it, and used the opportunity to also collapse the three
near-duplicate loops into one small `equipStarterItem` lambda —
removing the duplication is also what made the missing `break` obvious
in the first place.

## How I verified this

1. **`Damage.h` extraction verified behavior-neutral** — reran
   `combat_test.cpp` unchanged immediately after the refactor, before
   writing anything new; all 12 checks still passed.
2. **`melee_test.cpp`** (new, 6 checks) — verified the arc/range
   geometry by hand (a quick Python calculation) before trusting the
   test results, since an earlier test in this project had the *test's*
   geometry wrong, not the code. Confirmed: in-range/in-arc hits;
   in-range/out-of-arc (directly behind) misses; in-arc/out-of-range
   misses; a cleave hitting two simultaneous targets in one swing;
   cooldown correctly blocking an immediate second swing; and a kill
   via melee correctly spawning a corpse through the shared `Damage`
   path (confirming the `MeleeCombatSystem -> Damage -> spawnCorpse`
   wiring, not just melee's own geometry). Each test block uses its own
   fresh registry — an early draft shared one registry across blocks
   and would have had earlier blocks' surviving targets silently
   contaminate later swings, since the attacker never moves; caught and
   restructured before running it.
3. **`equipment_test.cpp`** (new, 11 checks) — equip/refuse-wrong-kind/
   refuse-non-weapon; swapping an already-equipped slot correctly
   returns the old weapon to the grid; equip correctly *refused* (not
   silently destroying anything) when the previously-equipped weapon
   has nowhere to go — this scenario's grid math was also hand-verified
   before encoding it; unequip working and correctly refused when the
   grid has no room; and `syncActiveWeapon` correctly populating
   `Weapon` XOR `MeleeWeapon` (never both, never stale data) across
   three different active-slot states.
4. Reran `combat_test.cpp`, `inventory_test.cpp`, `dragdrop_test.cpp`,
   `attachment_test.cpp` unchanged — all still pass.
5. Real `CMakeLists.txt` build (Ninja), headless run of the actual
   built binary — this specifically exercises the starter-loadout
   pipeline (3x `addItem`, 3x `equipStarterItem`, `syncActiveWeapon`)
   against the real `items.json` on disk, not a synthetic test
   database, with zero crashes.

Still no display — whether the melee arc (90°, 55px reach) feels right,
whether switching weapons via scroll feels natural, and how the SMG's
higher fire rate/lower damage/wider spread actually plays against the
pistol are all "does it feel right" questions worth you checking
directly.
