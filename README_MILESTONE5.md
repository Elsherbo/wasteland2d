# wasteland2d — Milestone 5 delivery

Everything here was built and verified against **real Box2D 3.1.1** (built from
source, not the stale 2.4.x Ubuntu package), real SDL2/SDL2_image/tinyxml2/glm,
using your exact `-std=c++20 -Wall -Wextra -Wpedantic` bar. Zero warnings across
every new and existing file. This isn't hand-reviewed code you're the first to
compile — it's already built, run, and unit-tested.

## What to do with this

Drop these files into your tree at the matching paths (all paths below are
relative to your project root, matching your existing layout):

```
game/components/Health.h        (new)
game/components/Weapon.h        (new)
game/systems/CombatSystem.h     (new)
game/main.cpp                   (replaces yours)
engine/render/Camera.h          (replaces yours)
```

`combat_test.cpp` is a standalone, no-SDL test (same style as your Milestone
2/3 headless tests) — not part of the game build. See "How I verified this"
below for how to build and run it if you want to rerun it yourself.

If you already applied the 5 audit fixes from before, `game/main.cpp` here
already includes the `linearDamping` removal (audit fix #2) — you don't need
to reapply that separately, it's baked into this file.

**One thing to add to your build:** if you're on the explicit-source-list
CMake fix from the audit patches, add the two new `.cpp`-free headers don't
need listing (header-only), but if you added `game/components/*.cpp` or
`game/systems/*.cpp` conventions later, remember they're headers here, not
sources — nothing to add to `GAME_SOURCES`.

---

## What's in it

### Movement feel
Player velocity is now exponentially smoothed toward the input direction
(separate `playerAccel`/`playerDecel` rates) instead of snapped instantly —
same technique `Camera::followSmooth` already used, applied to velocity. Tune
`playerAccel`/`playerDecel` in `main.cpp` by feel; I seeded them at `22.0f`/
`16.0f` (slightly quicker to stop than to get going) as a reasonable
starting point, not a final answer.

### Aim + hitscan combat
- `Camera::screenToWorld()` — new inverse of `worldToScreen()`, used to
  convert the mouse position into a world-space aim direction independent of
  movement. Deliberately ignores camera shake (see below) so aim doesn't
  jitter with the screen.
- `game::components::Weapon` — `fireRate`, `damage`, `range`,
  `spreadDegrees`, plus a `cooldown` owned entirely by `CombatSystem`.
- `game::components::Health` — `current`/`max`/`dead`, plus a free
  `applyDamage()` function (clamped, sticky-dead).
- `game::systems::CombatSystem` — `updateCooldowns()` each tick, and
  `fireWeapon()` which gates on cooldown, applies random spread, raycasts via
  your existing `PhysicsWorld::raycast()`, damages whatever it hits if that
  entity has `Health`, and despawns it on death.

### Muzzle flash, screen shake, tracer
The flash and shake are the "cheap juice" ROADMAP.md calls for — plain
`Sprite.color` tricks and one new generic `Camera::setShakeOffset()`, no new
rendering pipeline. Shake decays over its timer; magnitude is bigger on a
kill than a regular hit.

A tracer line (`tracerStart`/`tracerEnd`/`tracerTimer` in `main.cpp`, drawn
with a plain faded `SDL_RenderDrawLineF` after `SpriteRenderSystem::render`)
was added after the first round of feedback — hitscan resolves the shot
instantly, so without *something* drawn along the ray, a shot is completely
invisible. This isn't a projectile (no travel time, nothing simulated) —
purely a "here's where that went" flash, gone in 0.05s. Real projectile
weapons stay a later, separate `Weapon` type per ROADMAP.md.

### Target dummy
Static entity with `Health`, wired into the same darken-toward-black-as-
it-takes-damage loop that runs on anything with a `Health` component — so
this isn't dummy-specific code, it'll pick up any future `Health`-bearing
entity automatically.

---

## A real bug I found and fixed while testing this

`CombatSystem::fireWeapon()`'s death handling originally did:

```cpp
if (wasAlive && health.dead) {
    result.killedTarget = true;
    registry.destroy(target_e);
}
```

This destroys the **ECS entity** but never touches the **Box2D physics
body** it owned. `registry.destroy()` strips the entity's `RigidBody`
*component*, but does nothing to the actual `b2Body` that component was
wrapping — Box2D doesn't know the entity died. Net effect: killing something
leaves an invisible, un-lootable, permanently-solid piece of geometry sitting
in the physics world forever. Raycasts keep hitting it (I caught this because
my test's 5th shot, fired at an already-dead target, still registered a hit
— it should have been a clean miss). And on any future *dynamic* body — an
enemy in Milestone 8, say — it'd keep blocking movement too, invisibly,
after death.

Fixed by destroying the physics body first:

```cpp
if (registry.has<engine::physics::RigidBody>(target_e)) {
    auto& rb = registry.get<engine::physics::RigidBody>(target_e);
    physics.destroyBody(rb);
}
registry.destroy(target_e);
```

Worth remembering for Milestone 6: when corpses replace outright despawn,
whatever spawns the corpse entity will need to either reuse this destroyed
body's slot or explicitly *not* destroy the body and instead disable/repurpose
it — don't let the corpse-spawn code accidentally reintroduce this bug by
skipping body cleanup a second way.

---

## Two things fixed after in-game feedback

1. **No visible bullets was expected, not a bug** — pure hitscan resolves
   instantly, nothing travels, so there was nothing to render. Added a
   short-lived tracer line (see above) so a shot is at least visible for a
   frame or two, without turning this into an actual projectile system.
2. **Mojibake in the kill-message console output (`ΓÇö`)** — I'd used a
   Unicode em dash (`—`) inside a `printf` format string. Every other
   console string in your codebase is plain ASCII on purpose; Windows
   console doesn't default to a UTF-8 codepage, so those bytes rendered as
   garbage there even though they'd have looked fine on Linux/macOS.
   Replaced with `--`. Worth keeping in mind for any future `printf`/`cout`
   text: no Unicode punctuation (em dashes, curly quotes, etc.) in anything
   that prints to console, even though your source comments use them freely
   — comments never hit the console, so they're unaffected.

---

## How I verified this

1. **Built Box2D 3.1.1 from source** (the Ubuntu package is 2.4.1 — a
   completely different, incompatible API — so this is a real test against
   the version your project actually targets, not a stand-in).
2. **Built the unmodified project first** as a baseline — confirmed it
   already compiled clean under your exact warning flags before I changed
   anything.
3. **Applied the Milestone 5 changes**, rebuilt — clean under
   `-Wall -Wextra -Wpedantic`, no warnings, matching your project's own bar.
4. **Ran the built game headlessly** (Xvfb + software renderer) for a few
   seconds — confirms window/renderer/map/physics/entity setup (including
   the new `Weapon`/`Health`/dummy wiring) all initialize and the main loop
   runs without crashing.
5. **Wrote and ran `combat_test.cpp`** — a standalone, no-SDL test exercising
   `CombatSystem` against a real `PhysicsWorld`: cooldown gating (a
   same-tick second shot is correctly blocked), damage accumulation across
   3 shots (60 → 35 → 10 → dead), death/despawn, and — after finding and
   fixing the bug above — confirmed a shot at an already-dead target is a
   clean miss instead of hitting a ghost body. All 5 assertions pass.

I did not have a display available to test mouse-aim or manually verify the
shake/flash *look* right — that's the one thing worth eyeballing yourself
once you build it. Everything else (the logic, the math, the integration) is
verified, not just "should work."
