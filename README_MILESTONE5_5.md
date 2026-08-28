# wasteland2d — Milestone 5.5 delivery

Same bar as Milestone 5: built and verified against **real Box2D 3.1.1**
(built from source), real SDL2/SDL2_image/tinyxml2/glm, your exact
`-std=c++20 -Wall -Wextra -Wpedantic`. Zero warnings across every new and
existing file. Two standalone tests, both passing.

## What to do with this

Drop these in at the matching paths (all relative to your project root):

```
game/main.cpp                        (replaces yours)
game/components/Health.h             (unchanged from Milestone 5)
game/components/Weapon.h             (unchanged from Milestone 5)
game/systems/CombatSystem.h          (unchanged from Milestone 5)

engine/ecs/Attachment.h              (new)
engine/ecs/AttachmentSystem.h        (new)
engine/fx/FlashEffect.h              (new)
engine/fx/ScreenShake.h              (new)
engine/fx/WorldLineEffects.h         (new)
engine/input/Cursor.h                (new)
engine/render/Camera.h               (replaces yours)
engine/render/Sprite.h               (replaces yours)
engine/render/SpriteRenderSystem.h   (replaces yours)
engine/resource/TextureCache.h       (replaces yours)
engine/world/TileMap.h               (replaces yours)
engine/world/TileMap.cpp             (replaces yours)
```

`tests/combat_test.cpp` and `tests/attachment_test.cpp` are standalone,
no-SDL tests (the same style as your own Milestone 2/3 headless tests) —
not part of the game build. See "How I verified this" for how to build
and run them.

`game/components/Health.h`, `Weapon.h`, and `game/systems/CombatSystem.h`
are included unchanged from the Milestone 5 delivery, just so this package
is self-contained — you don't need to go find them from the earlier zip.

**CMake note:** all six new files are headers, so if you're on the
explicit-source-list fix from the original audit, nothing needs adding to
`ENGINE_SOURCES`/`GAME_SOURCES` — headers aren't listed there, only `.cpp`
files.

**Breaking API change worth knowing about:** `TileMap`'s constructor now
takes `resource::TextureCache&` instead of `SDL_Renderer*` (see below) —
`main.cpp` here already reflects that, but if you have anything else
constructing a `TileMap` directly, it needs the same change.

---

## What's in it, mapped to what you originally asked for

- **"can have multiple sprites (hair/head/pants/etc.)"** → `ecs::Attachment`
  + `AttachmentSystem`. The player is now one root entity (movement,
  physics, combat) plus three attached entities (hair, pants, weapon),
  each an ordinary `Transform`+`Sprite` entity that `SpriteRenderSystem`
  already knew how to draw — no special-cased multi-sprite rendering
  needed. Still colored rectangles, not real art — see below.

- **"weapons will have its own sprite, and the weapon should rotate with
  the mouse"** → the equipped weapon is an `Attachment` with
  `followRotation = false`, so `AttachmentSystem` keeps its *position*
  pinned to the player but never touches its *rotation* — `main.cpp` sets
  `Transform.rotationDegrees` on it directly from the same `aimDir` vector
  `CombatSystem` already uses, every frame. Build and aim the mouse around
  the player — the weapon rectangle visibly tracks it, independent of
  movement direction.

- **"change the cursor based on the weapon or back to cursor if inventory
  is open"** → `engine::input::Cursor` is the *capability* (hide the OS
  cursor, draw any texture at the mouse position instead) — demoed here
  as a small tinted square via the same `whitePixel()` placeholder every
  other entity in this file uses. The *policy* of switching cursors by
  weapon or by UI state needs a real trigger condition to hook into —
  weapon-switching needs Milestone 6's equipment slots, and
  inventory-open needs Milestone 6's UI. Building the policy now would
  mean guessing at an interface that doesn't exist yet.

- **"slight camera offset where the mouse is aiming"** →
  `Camera::followSmoothWithOffset()`, biasing the usual follow target
  toward `playerPos + aimDir * cameraAimOffsetPixels`, smoothed at its
  own (slower) rate so quick aim flicks don't visibly jitter the camera.
  Tunable via `cameraAimOffsetPixels`/`cameraAimOffsetLerpSpeed` in
  `main.cpp`.

- **"game juice system for our framework"** → `engine::fx`
  (`FlashEffect`/`FlashSystem`, `ScreenShake`, `WorldLineEffects`).
  Milestone 5's three hand-rolled timer blocks
  (`muzzleFlashTimer`, `shakeTimer`/`shakeMagnitude`,
  `tracerStart`/`tracerEnd`/`tracerTimer`) are gone from `main.cpp` —
  replaced by these three reusable primitives. **Visible behavior is
  unchanged from Milestone 5** — this part is a refactor, not a new
  effect, on purpose (see the ROADMAP.md revision I sent earlier for the
  reasoning: infrastructure now, new effect *content* — particles,
  damage numbers — stays in Milestone 12).

- **Real texture rendering** → `TextureCache::load(path)` (backed by
  SDL2_image, already a dependency — zero new dependencies added) and
  `Sprite::texture`/`sourceRect`. `TileMap`'s tileset image now loads
  through the same cache instead of owning its own `SDL_Texture`
  independently — one texture-owning path in the engine, not two.
  **Nothing in this delivery actually uses a real texture yet** — every
  entity still deliberately uses the colored-rectangle placeholder path
  (`Sprite::texture` left `nullptr`). This milestone builds the
  capability; real character/weapon art is Milestone 6+ content, same
  distinction as the cursor above.

---

## The one thing I did *not* build here, and why

Primary/secondary/melee weapon slots aren't in this delivery. Per the
ROADMAP.md revision, that's deliberately sequenced as the first real use
of Milestone 6's equipment-slot system, built on actual item data — not
bolted onto Milestone 5's single hardcoded `Weapon` component now and
rebuilt again once inventory exists. If you want it sooner than that,
say so and I'll revisit the sequencing, but I'd only build it now knowing
it's very likely getting rebuilt in a few weeks.

---

## How I verified this

1. **Built Box2D 3.1.1 from source** again (same as Milestone 5 — Ubuntu's
   package is 2.4.1, a different, incompatible API).
2. **Applied all Milestone 5.5 changes on top of the working Milestone 5
   baseline**, rebuilt — clean under `-Wall -Wextra -Wpedantic`, zero
   warnings, every file (new and modified).
3. **Reran the Milestone 5 standalone `combat_test.cpp` unchanged** — all
   5 assertions still pass. Nothing about `CombatSystem`'s logic changed
   in this milestone, and this confirms it.
4. **Wrote and ran a new standalone `attachment_test.cpp`** — no SDL, no
   Box2D, just `Registry`/`Transform`/`Attachment` (even less dependency
   surface than `combat_test.cpp`, since `AttachmentSystem` doesn't touch
   physics or rendering at all). Verifies: a `followRotation = true`
   attachment's position rotates correctly with its parent and copies the
   parent's rotation exactly (checked against the actual trig, not just
   "some plausible number"); a `followRotation = false` attachment's
   position still follows but its rotation is left completely untouched
   (checked against a sentinel value that would only survive if the
   system genuinely never writes it); and destroying the parent mid-run
   doesn't crash and leaves the attached entity exactly where it last
   was rather than snapping somewhere wrong. All 3 checks pass.
5. **Ran the built game headlessly** (Xvfb + software renderer) for a few
   seconds — confirms the full entity setup (player + 3 attachments +
   tree + vehicle + dummy, texture-cache-backed tilemap, cursor
   initialization) starts and the main loop runs without crashing.

I still don't have a display to eyeball the actual look — whether the
camera lean feels right, whether the paperdoll rectangles read as
sensible without real art, whether 6x6 pixels is a reasonable cursor
size. All of that is genuinely a "does it feel right" question, not a
correctness one, and worth you looking at directly once it's built.
