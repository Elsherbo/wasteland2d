# wasteland2d — Milestone 6, drag-and-drop UI

This is the piece deliberately deferred from the last delivery — completes
Milestone 6. Verified through the real `CMakeLists.txt` again (Ninja
configure + build, zero manual workarounds), all 4 standalone tests passing
(31 assertions total), headless run stable.

## What to do with this

```
game/main.cpp                        (replaces yours)
engine/input/InputManager.h          (replaces yours)
engine/input/InputManager.cpp        (replaces yours)
engine/ui/GridLayout.h               (new)
engine/ui/GridRenderer.h             (new)
game/ui/DragDropController.h         (new)
game/ui/InventoryRenderer.h          (new)
```

`tests/*.cpp` — `dragdrop_test.cpp` is new; the other three are included
unchanged, just so this package is self-contained. No `CMakeLists.txt`
change needed this time — everything new here is a header, and
`InputManager.cpp` already exists as a tracked source.

## What it does

Press **Tab** to open your inventory — a grid panel appears (top-left),
showing your items as tinted rectangles. Click and drag a stack to
rearrange it within your own inventory. Walk near the target dummy's
corpse (or anything else with a `Lootable` + `Inventory`) while the panel
is open, and a second panel appears next to it — drag items from the
corpse into your own inventory, or vice versa, cell by cell. Release a
drag anywhere invalid (outside both panels, or a spot that doesn't fit)
and the item snaps back to exactly where it was — nothing is ever lost.
Drop an item's footprint directly onto a compatible existing stack (same
item, exact position, room left) and they merge instead of colliding.
Movement, aim, ADS, and firing are all suspended while the panel is open
— same "menu-open pause" convention Zero Sievert/Tarkov use — and the
custom aim cursor swaps to the OS cursor while it's open, back to the
crosshair when you close it.

The console-based "press E to loot everything" from the last delivery
is untouched and still works exactly as before — this adds the granular
option alongside it, doesn't replace it.

## Two things worth knowing about before you build this

**A real engine bug got fixed to make this possible.** `InputManager`
only ever tracked mouse buttons as "currently held" — `beginFrame()`'s
mouse-edge-tracking loop was a literal no-op, its own comment saying so
("query held state only for now"). Drag-and-drop fundamentally needs
"button just went down" and "button just went up" as distinct one-frame
events, not continuous held state — without that, letting go of the mouse
would have no reliable single moment to actually commit a drop. Extended
mouse buttons to use the same `held`/`pressedThisFrame`/`releasedThisFrame`
pattern already built for keyboard `Action`s, and added
`wasMouseButtonPressed`/`wasMouseButtonReleased` alongside the existing
`isMouseButtonHeld` (which still behaves identically to before).

**No text rendering exists anywhere in this engine.** No font system, no
SDL_ttf integration — nothing renders text at all, anywhere, in any
milestone so far. So this UI genuinely cannot show item names, stack
quantities, or a weight/capacity readout. A stack of 30 9mm rounds and a
single bandage both look like one tinted rectangle; you can't currently
tell "how many" just by looking. This isn't a corner I cut inside the
UI — it's a missing prerequisite *engine* capability that this milestone
doesn't include. Worth deciding whether you want that as the next
increment, since a fair amount of remaining polish (tooltips, prompts
that aren't console-only, a real HUD) all sit behind the same gap.

## Design choices worth flagging

- **`game::ui::DragDropController` lives in `game/`, not `engine/`** —
  same reasoning as `InventorySystem`/`CombatSystem`: it references
  `Inventory`/`ItemDatabase` directly, and a genuinely generic,
  abstracted `engine::ui` drag-and-drop framework would be guessing at
  what a different game's grid UI wants, with no second project to
  validate that guess against. What's actually generic here
  (`GridLayout`'s screen↔cell math, `GridRenderer`'s cell-background
  drawing) already lives in `engine/ui/`.
- **Merge-on-drop only triggers at an exact position match** — dropping
  a stack of arrows anywhere inside an existing arrow stack's footprint
  *except* precisely its top-left corner is treated as a normal
  (colliding, thus invalid) placement, not a merge. A more forgiving
  "anywhere overlapping" merge rule is possible but meaningfully more
  code for a UX difference you may or may not want — happy to add it if
  the exact-position requirement feels wrong once you're using it.
- **A partial merge can return less than the full original stack to your
  cursor's origin** — if you drag 3 arrows onto a stack that only has
  room for 1, that 1 merges in and the other 2 snap back to where you
  picked them up from, not to your cursor. Tested explicitly (see
  `dragdrop_test.cpp`); flagged here because it's a genuine behavior
  choice, not just an implementation detail — a different game might
  want the leftover to end up wherever you release the mouse instead.

## How I verified this

Same rigor as every delivery so far:
1. Rebuilt the real `CMakeLists.txt` end-to-end (Ninja, zero workarounds).
2. **`dragdrop_test.cpp`** (8 checks, new) — `GridLayout`'s screen↔cell
   math at and past every edge; picking up a shaped item via a
   non-origin cell within its footprint; refusing a second pickup mid-drag;
   refusing pickup on an empty cell; a clean move to empty space; an
   invalid (colliding) drop correctly returning the stack to its exact
   original position; a full merge; and the partial-merge-with-leftover
   case described above, checked down to exact quantities on both sides.
3. Reran **`combat_test.cpp`**, **`inventory_test.cpp`**, and
   **`attachment_test.cpp`** unchanged — all still pass, confirming
   nothing about corpse-spawning, inventory placement/weight math, or
   attachments regressed.
4. Headless run of the CMake-built binary — confirms the full UI setup
   (panel layouts, drag controller, cursor-swap wiring) initializes and
   the main loop runs without crashing.

I still don't have a display — whether the panel positions look right,
whether 44px cells feel proportioned well against a 6x4 grid, and
whether dragging actually *feels* responsive are all things worth you
checking directly. The logic underneath is genuinely tested; the layout
numbers are reasonable guesses, same caveat as every other untuned
constant so far.
