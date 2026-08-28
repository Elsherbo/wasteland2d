# wasteland2d — ADS (aim-down-sights) zoom & offset

Small, focused addition on top of Milestone 5.5 — three files changed, no
new files. Built and verified the same way as everything else so far: real
Box2D 3.1.1, clean under `-Wall -Wextra -Wpedantic`, both standalone tests
(`combat_test.cpp`, `attachment_test.cpp`) still pass unchanged, headless
run stable.

## What changed

- **`engine/render/Camera.h`** — new `zoomSmooth(targetZoom, dt, lerpSpeed)`,
  same exponential-smoothing technique as `followSmooth`, applied to zoom
  instead of position. `followSmoothWithOffset` is unchanged — it's now
  just called conditionally instead of every frame.
- **`game/components/Weapon.h`** — two new fields:
  `adsZoomMultiplier` (default `1.15f`) and `adsOffsetPixels` (default
  `50.0f`). Deliberately per-weapon, not one global constant — this is
  exactly the field a real AR or sniper `Weapon` instance would set
  differently once Milestone 6's equipment slots let more than one
  `Weapon` exist at a time. Defaults here are pistol-ish: mild zoom,
  modest lean.
- **`game/main.cpp`** — the constant, always-on camera lean from
  Milestone 5.5 is gone. Camera offset/zoom now only move while
  right-click is held (`input.isMouseButtonHeld(SDL_BUTTON_RIGHT)`),
  pulling their strength from the player's `Weapon` component, and
  smoothly relax back to zoom `1.0` / no lean when released.

## Why the defaults are what they are

I don't have a display, so `1.15f` zoom / `50px` lean are reasonable
starting guesses for a pistol, not tuned by feel — the same caveat as
`playerAccel`/`playerDecel` back in Milestone 5. Once you've got this
running, adjust `Weapon::adsZoomMultiplier`/`adsOffsetPixels` directly
(currently only set via the default member initializers, since there's
still only one `Weapon` instance in the game).

## Worth knowing: this only has one weapon to test against

Since multi-weapon loadout is still correctly deferred to Milestone 6 (per
our earlier roadmap discussion), there's no AR or sniper instance anywhere
in the codebase yet to actually demonstrate "AR zooms more than pistol."
What's proven here is that the *mechanism* reads its zoom/offset strength
from whatever `Weapon` the player has equipped, rather than a hardcoded
global — so when Milestone 6 adds a second and third `Weapon` with
different `adsZoomMultiplier`/`adsOffsetPixels` values, switching the
equipped slot will make ADS behave differently with zero changes to this
code. I'd treat "does an AR actually feel different from a pistol" as a
Milestone 6 acceptance question, not something to fake here with two
hardcoded weapon instances that don't fit anywhere in the entity setup yet.
