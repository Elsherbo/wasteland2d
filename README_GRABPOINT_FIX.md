# wasteland2d — grab-point-relative dragging

Small, focused fix on top of inventory UI v2. 4 files changed, no new
files. Same verification bar: real `CMakeLists.txt` build, headless run,
all standalone tests passing (`dragdrop_test.cpp` now has 15 checks, up
from 13 — 2 new for this specifically).

## What changed

Picking up a shaped item used to always snap its **top-left corner**
under the cursor, regardless of where on the item you actually clicked.
Grab a 2x2 crate by its bottom-right cell and the whole thing would jump
so that cell became the new top-left — visually wrong, and not how any
polished grid inventory (Tarkov, Diablo, etc.) actually behaves.

- **`HeldStack`** gained `grabOffsetX`/`grabOffsetY` — which cell within
  the item's footprint was clicked, relative to its top-left. Recorded
  in `beginDrag()`, and its axes swap along with the footprint on
  `toggleRotation()`.
- **`DragDropController::resolveDropTopLeft(hoverX, hoverY)`** — new
  method, the one place "mouse is over this cell" gets converted into
  "the item's top-left would land here." `previewDrop()`/`endDrag()`
  still just take an already-resolved top-left directly (their
  semantics didn't change) — callers resolve first, then act, which is
  why the existing tests needed zero changes.
- **`InventoryRenderer::renderHeldStack`** — the drag ghost now follows
  the mouse in smooth pixel space, offset by the grab point in pixels
  (`mouseX - grabOffsetX * cellSize`), not grid-snapped. The grid-
  snapped *landing* preview is a separate concern — that's what the
  green/red highlight already shows, via `resolveDropTopLeft()`.
- **`main.cpp`**'s two call sites (the actual `endDrag` on mouse-release,
  and the highlight/preview) both now call `resolveDropTopLeft()` first.

## A bug I found in my own test while verifying this — worth knowing about

First version of the new test picked up a 2x2 crate by its bottom-right
cell in a **4x4** grid, then asserted it would land at the resolved
position (4,4). That's out of bounds for a 2x2 item in a 4x4 grid
(4+2=6 > 4) — so the drop was correctly rejected as invalid, and the
crate correctly snapped back to its original position instead. The
*code* was right; my test's grid size was too small for the position I
picked. Fixed by testing in a 6x6 grid instead, where (4,4) is a valid
corner placement. Same class of mistake as an earlier test (asserting a
4x4 grid could only fit two 2x2 crates when it actually fits four) —
worth double-checking grid arithmetic by hand before trusting a failing
assertion means the implementation is wrong.

## How I verified this

1. Real `CMakeLists.txt` build (Ninja), zero manual workarounds.
2. **`dragdrop_test.cpp`** (15 checks, +2 new): grabbing a 2x2 item
   off-center and confirming `resolveDropTopLeft` preserves that offset
   *and* that the item actually lands there after `endDrag` — not just
   that the math works in isolation; and confirming the grab offset's
   axes correctly swap when the item is rotated mid-drag.
3. All other tests (`combat_test.cpp`, `inventory_test.cpp`,
   `attachment_test.cpp`) rerun unchanged — nothing about this touched
   their logic.
4. Headless run of the CMake-built binary.

Still no display — whether the pixel-smooth ghost tracking actually
*feels* right (versus, say, feeling like it drifts) is worth confirming
directly once you've got it running.
