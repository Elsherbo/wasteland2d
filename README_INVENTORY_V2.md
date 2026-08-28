# wasteland2d — inventory UI v2: rotate, swap, quick-transfer, highlights

Same verification bar as every delivery so far: real `CMakeLists.txt`
build, headless run, and now **4 standalone tests, 38 assertions total**
(dragdrop_test.cpp grew from 8 to 13 checks; inventory_test.cpp grew from
8 to 12).

## Is this modular and reusable, for the engine, not just this game?

Genuinely yes for part of it, and deliberately no for another part —
here's the honest breakdown, not a blanket "yes":

**Actually engine-level and reusable across any game:**
- `engine::ui::GridLayout` — pure screen↔cell math, zero game-specific
  types. Any grid UI (inventory, crafting, a hotbar) in any game reuses
  this unchanged.
- `engine::ui::GridRenderer` (`renderGridCells`, `renderHighlightRect`) —
  generic cell/highlight drawing, takes colors as parameters, has no
  opinion on what's *in* a cell.
- `engine::input::InputManager`'s press/release edges and `isCtrlHeld()` —
  general input infrastructure, not inventory-specific at all.

**Game-level on purpose, not a shortcut:**
- `game::components::Inventory`, `game::systems::InventorySystem`,
  `game::ui::DragDropController` all reference `Inventory`/`ItemDatabase`
  directly. A truly generic, abstracted `engine::ui` drag-and-drop
  *framework* would mean guessing at what a completely different game's
  grid UI wants, with no second project to validate that guess against
  — that's premature abstraction, not a shortcut, and every other
  genuinely game-specific piece in this codebase (`CombatSystem`,
  `Health`, `Weapon`) already draws this same line. This isn't a gap;
  it's the same deliberate split the whole project has used since
  Milestone 5.

**The good news, concretely:** the earlier design held up under real
pressure. `InventorySystem::canPlace` already took `width`/`height` as
explicit parameters instead of deriving them from an item id — which is
exactly what made rotation *additive* (new stacks just pass swapped
dimensions) rather than a rewrite. And `DragDropController` needed a
real refactor for this pass (see below), but the refactor was about
*correctness* — avoiding the preview and the actual drop logic drifting
apart — not about the original design being wrong.

## What's new

- **Rotation** — press **R** while dragging. `InventoryStack` gained a
  `rotated` bool; `InventorySystem::canPlace` now checks each existing
  stack's *effective* (rotation-swapped) footprint. Manual/drag-only —
  automatic placement (looting, corpse spawn) never rotates anything on
  its own; see `Inventory.h`'s comment for why that's a deliberate line,
  not an oversight.
- **Swapping** — drop a held item onto exactly one other stack whose
  footprint *exactly* matches (same position, same effective size) and
  they trade places, instead of the drop just failing. Works both
  within one inventory (reorganizing) and across two (player ↔ a
  lootable) — both paths are explicitly tested. Restricted to exact
  matches on purpose: a resize-and-relocate swap for differently-sized
  items is a real, larger feature, not attempted here.
- **Quick-grab/quick-store** — **Ctrl+click** a stack to instantly send
  it to "the other" visible panel, no drag needed. Built on a new
  `InventorySystem::quickTransferStack`, which reuses the same
  partial-transfer, nothing-lost logic `moveAllTo` already had.
- **Hover highlighting + drop-preview** — hovering shows what's under
  the cursor; dragging shows green (valid) or red (invalid) for the
  *exact* position the mouse is over.

## The one refactor this pass needed, and why

`DragDropController::endDrag` previously decided merge/place/reject
inline, in one function. Adding swap as a third outcome, on top of an
already-growing decision tree, made it worth asking: how do I show an
accurate green/red preview *before* the mouse is released, without
maintaining two separate copies of the same logic that could quietly
drift apart?

Split it into `planDrop()` (pure, read-only — returns what *would*
happen: `Invalid`/`Place`/`Merge`/`Swap`, plus which existing stack is
involved) and `endDrag()` (calls `planDrop()`, then mutates based on the
result). `previewDrop()` is now just `planDrop().outcome`. There is
exactly one place this decision gets made — the preview and the real
drop are structurally incapable of disagreeing, not just carefully kept
in sync by hand.

## How I verified this

1. **Real `CMakeLists.txt` build** (Ninja), zero manual workarounds.
2. **`dragdrop_test.cpp`** (13 checks, +5 new): rotation changing whether
   a 1x3 item fits a 1-row grid (and that the actual placement afterward
   matches what the preview said); a cross-inventory swap; a
   same-inventory swap (the trickier case — same underlying vector
   mutated twice in sequence, checked for correctness, not just "didn't
   crash").
3. **`inventory_test.cpp`** (12 checks, +4 new): `stackIndexAt`/
   `removeStackAt` via a non-origin cell and on an already-empty cell;
   `quickTransferStack` both fully succeeding and correctly leaving the
   source completely untouched when the destination has zero room.
4. **`combat_test.cpp`** and **`attachment_test.cpp`** rerun unchanged —
   confirm rotation-aware `canPlace` didn't regress anything (every
   existing test only ever uses `rotated = false`, its default, so
   behavior for them is bit-for-bit the same as before).
5. Headless run of the CMake-built binary.

Still no display on my end — the actual *feel* of rotating, swapping,
and quick-transferring, and whether the highlight colors read clearly
against the current colored-rectangle placeholders, are worth you
checking directly.
