# wasteland2d — backpack capacity, encumbrance, hotbar, equipment UI

Completes the three-part ask from last time: visible equipment slots,
a real hotbar for consumables, and DayZ/Zero-Sievert-style backpack
capacity with a movement penalty. Same bar as every delivery: real
`CMakeLists.txt` build, headless run (full startup pipeline — backpack
equip, bandage pickup, quick-slot binding — against the real
`items.json`), and now **eight** standalone test suites, three new this
pass (`encumbrance_test.cpp`, `useitem_test.cpp`, plus backpack cases
added to `equipment_test.cpp`), 24 new assertions total.

## What to do with this

```
game/main.cpp                        (replaces yours)
engine/input/InputManager.h          (replaces yours)
engine/input/InputManager.cpp        (replaces yours)
game/components/EquipmentSlots.h     (replaces yours — adds Backpack slot)
game/components/Inventory.h          (replaces yours — adds softMaxWeight)
game/components/QuickSlots.h         (new)
game/data/ItemDefinition.h           (replaces yours — adds carryCapacityKg, UseEffect)
game/data/ItemDatabase.cpp           (replaces yours)
game/systems/EquipmentSystem.h       (replaces yours — backpack capacity logic)
game/systems/InventorySystem.h       (replaces yours — two new small helpers)
game/systems/Encumbrance.h           (new)
game/systems/UseItemSystem.h         (new)
game/ui/EquipmentRenderer.h          (new)
assets/items/items.json              (replaces yours — two backpack tiers, bandage heals)
```

No `CMakeLists.txt` change needed.

## One assumption I made without a confirmed answer

I asked whether to allow carrying past capacity (DayZ-style, with a
movement penalty) or keep pickup hard-blocked and only scale speed near
the cap. You replied "continue" rather than picking an option — your
own original phrasing ("a small weight before he can barely walk...
can't move at all with a large backpack") only really describes the
first model, so that's what I built. **Flagging this plainly because
it's a real assumption, not a confirmed decision** — if you actually
wanted the hard-blocked version, that's a smaller, different change
(mainly reverting `Inventory::softMaxWeight` back to a single
`maxWeight`, and dropping `Encumbrance`'s interpolation for a simpler
"speed scales as you approach the one cap" curve) — say so and I'll
redo it.

## Controls

- **B** while the inventory is open and hovering a backpack: equips it.
  No numbered hotkey — 1-3 are weapons, 4-9 are quick slots, so
  backpack needed its own key.
- **Ctrl + 4-9** while the inventory is open and hovering a usable item
  (has a `useEffect` — a bandage, say): binds it to that hotbar slot.
- **4-9** with the inventory **closed**: uses whatever's bound there.
- The hotbar (bottom-center, always visible) shows the key number, a
  tinted swatch for what's bound, and the *total* count across every
  matching stack — not just one stack's quantity.
- Equipment slots render as a 4-box row above your inventory grid when
  it's open; the active weapon slot gets a gold highlight.

Player starts with a school backpack equipped and 3 bandages bound to
key 4, on top of the existing pistol/SMG/knife loadout.

## The weight model, concretely

- **Soft cap** = 5kg base ("pockets only") + your backpack's stated
  capacity (school: 15kg, military: 60kg). Crossing it never blocks
  anything — it's purely the point where the speed penalty starts.
- **Hard cap** = soft cap x 1.5. This is the actual pickup gate —
  `InventorySystem::addItem`'s existing logic is completely unchanged,
  it just now checks a bigger number when you're carrying a real
  backpack.
- Between the two, speed drops linearly from 100% to a 5% floor (never
  literally 0 — a true freeze reads as a bug, not a mechanic).
- **Downgrading to a smaller backpack while already over its capacity
  is allowed, and does apply the new (smaller) caps immediately** —
  including if that leaves you instantly over the new hard cap. This
  is intentional: swapping to a smaller bag while loaded should have a
  real consequence, not silently rescue you from being overloaded.
  Covered explicitly in `equipment_test.cpp`.

## How I verified this

1. **`equipment_test.cpp`** (+3 backpack checks, 14 total) — equipping
   sets soft/hard caps correctly (`base + capacity`, `soft x 1.5`);
   unequipping resets to base; the downgrade-while-loaded scenario
   applies the new caps immediately regardless of current load.
2. **`encumbrance_test.cpp`** (new, 5 checks) — full speed under the
   soft cap, including exactly at the boundary; the halfway point
   between soft and hard cap checked against a hand-computed expected
   value (0.525), not just "some number less than 1"; beyond the hard
   cap floors at the minimum rather than continuing past it; a
   degenerate configuration (hard cap at or below soft cap) fails safe
   instead of dividing by a non-positive range.
3. **`useitem_test.cpp`** (new, 7 checks) — heal amount applied and
   clamped to max Health; the actual healed delta reported, not the
   nominal amount (matters when clamping cuts it short); the last unit
   of a stack removes the stack entirely rather than leaving a
   quantity-0 ghost; using with none left, an unassigned slot, an item
   with no `UseEffect`, and a dead entity are all correctly refused —
   the dead-entity case specifically confirms the item isn't wasted
   for zero effect, not just that nothing happens.
4. Reran all five prior suites (`combat_test.cpp`, `melee_test.cpp`,
   `inventory_test.cpp`, `dragdrop_test.cpp`, `attachment_test.cpp`)
   completely unchanged — all still pass, confirming the
   `Inventory::maxWeight` default change (40kg to 7.5kg, now meaning
   "base hard cap with no backpack") didn't silently break anything;
   every existing test already set `maxWeight` explicitly, so this was
   verified rather than just assumed safe.
5. Real `CMakeLists.txt` build (Ninja), headless run of the actual
   built binary — exercises the complete new startup pipeline (equip
   backpack, add bandages, bind hotbar slot, compute initial
   encumbrance) against the real `items.json`, zero crashes.

Still no display — whether the 1.5x overload multiplier feels right,
whether the equipment slot row's position/size reads well, and how the
hotbar looks against the game world are all worth you checking
directly. The numbers (5kg base, 15/60kg backpacks, 5% speed floor)
are reasonable starting guesses, same caveat as every other untuned
constant so far.
