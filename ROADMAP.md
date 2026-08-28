# wasteland2d — Project Roadmap

This is the full build plan for wasteland2d: a reusable C++20 2D top-down
framework, developed alongside a first game in the Ashworld / Zero Sievert
tradition (irradiated wasteland survival, scavenging, tension-driven
top-down combat and vehicle traversal).

The project is deliberately built in two layers from milestone one:

- **`engine/`** — the framework. Everything in here is generic: it has no
  concept of "radiation," "loot," or "mutants." This is what future
  projects reuse wholesale.
- **`game/`** — this specific game. Everything Ashworld/Zero-Sievert-
  specific lives here, built *on top of* the engine, never inside it.

The test applied at every milestone: **could this code ship in a
completely different top-down game unchanged?** If yes, it belongs in
`engine/`. If no, it belongs in `game/`. This is what keeps the framework
honest instead of slowly turning into "the wasteland game with some
folders renamed."

Each milestone below is scoped to be independently buildable and
independently useful — you should have a runnable, meaningfully-improved
game at the end of every single one, never a multi-week gap with nothing
to run.

---

## Milestone 1 — Core loop, window, input ✅ *Complete*

**Goal:** the absolute foundation every other milestone builds on.

- SDL2 window + renderer (`engine::Window`), hardware-accelerated with
  software fallback.
- Fixed-timestep game loop (`engine::Application`, `engine::Clock`) —
  simulation runs at a constant 60Hz regardless of render/display rate,
  which matters the moment physics or networked-feeling determinism is
  involved. Render is interpolated against an `alpha` value for
  smoothness independent of sim rate.
- Action-mapped input (`engine::InputManager`) — gameplay code asks
  `isHeld(Action::MoveUp)`, never a raw `SDL_SCANCODE_W`. Rebinding keys
  later is a config change, not a code change across every system that
  reads input.
- Zero-warning build under `/W4` (MSVC) and `-Wall -Wextra -Wpedantic`
  (GCC/Clang).

**Verified:** builds and runs on Windows (MSVC/vcpkg) and Linux
(apt/pkg-config); a WASD-movable placeholder square confirmed input,
loop timing, and window/renderer setup all work end-to-end.

---

## Milestone 2 — ECS, sprite rendering, camera ✅ *Complete*

**Goal:** turn "one hardcoded square" into a real entity/component
architecture that every future system plugs into.

- **Sparse-set ECS** (`engine::ecs::Registry`) — `Entity` is a packed
  32-bit index+generation handle, so a stale handle to a destroyed
  entity can't silently alias a new entity that reuses the same slot
  (critical once bullets, corpses, and dropped loot are constantly
  spawning/dying). `ComponentPool<T>` gives O(1) add/remove/has and
  tightly-packed iteration. `Registry::view<Ts...>()` returns entities
  matching an arbitrary component combination.
- **`Transform`** (position, rotation, scale) and **`Sprite`** (size,
  color, draw layer) components — both engine-generic.
- **`Camera`** — world↔screen transform with zoom and exponential
  smooth-follow (framerate-independent).
- **`SpriteRenderSystem`** — layer-sorted draw pass over
  `view<Transform, Sprite>()`.
- **`TextureCache`** — currently hands out one shared tinted white-pixel
  texture (colored-rectangle placeholders); structured so real texture
  loading slots in without touching call sites.

**Verified:** a standalone unit test (no SDL dependency) confirmed
multi-component view filtering, swap-and-pop removal, and generation-safe
entity reuse. Full project built and ran headless against real SDL2.

---

## Milestone 3 — Tilemaps, collision ✅ *Complete*

**Goal:** replace placeholder geometry with an actual level you can walk
around and be blocked by.

- **`engine::world::TileMap`** — loads Tiled (`mapeditor.org`) `.tmx`
  maps via `tinyxml2`: one embedded tileset image, CSV-encoded tile
  layers, and a `collision` object layer (plain rectangles) for level
  geometry. Scope is intentionally narrow (no external `.tsx`, no
  Base64/zlib layer compression) to avoid pulling in a compression
  dependency for a framework this size — documented directly in
  `TileMap.h` so it's a deliberate, revisitable choice rather than a
  silent limitation.
- **`engine::physics::AABB` / `Collider`** — axis-aligned bounding boxes;
  `Collider` is a separate component from `Sprite` since visual size and
  collision footprint often differ.
- **Axis-separated collision resolution** in the player's move step (try
  X, revert if blocked; try Y, revert if blocked) — the standard
  top-down approach that produces sliding-along-walls instead of
  sticking the instant movement isn't perfectly wall-parallel.

**Verified:** a standalone test loaded the real sample map and checked
`collides()` at four known points (open spawn, a wall corner, an
interior block, open floor) — all four matched expectations. Full
project built and ran headless with the real map loaded.

---

## Milestone 4 — Physics foundation: GLM, Box2D, Y-sorting, triggers

**Goal:** replace the hand-rolled AABB/axis-separated collision from
Milestone 3 with a real physics world, and add the two rendering/
detection primitives that every later milestone (combat, vehicles, AI,
interaction prompts) depends on. This is infrastructure, not a visible
gameplay milestone on its own — but every milestone after it is built
wrong if this one is skipped or shortcut.

- **GLM** (`vec2`, `normalize`, `rotate`, `lerp`, ...) replaces raw
  `double x, y` arithmetic scattered through movement/aim/camera code.
  Header-only, no build-system cost, immediate ergonomic win for the
  vector math combat and vehicle steering both need.
- **Box2D integration** (`engine::physics::PhysicsWorld`) — a `b2World`
  stepped once per fixed update, at **1 meter = 32 pixels** (matching
  tile size exactly, so tiles are clean 1m×1m units with no awkward
  conversion factor). Replaces `TileMap::collides()` and the axis-
  separated player movement from Milestone 3:
  - **Static bodies** — tilemap collision rectangles become Box2D static
    fixtures once, at map load.
  - **Dynamic body — player** — high linear damping, no angular physics,
    so it still *feels* like direct top-down movement rather than a
    physics object sliding around; velocity is set toward input
    direction each frame rather than driven by applied force.
  - **`RigidBody` component** wraps a `b2Body*` handle; a sync step after
    `world.step()` copies position/angle back into `Transform` for
    rendering. `Transform` becomes a read-only mirror of physics state
    for any entity with a `RigidBody` — the body is the source of truth,
    not the component.
- **Y-sorting** — `SpriteRenderSystem`'s draw order becomes a two-key
  sort: primary key `Sprite.layer` (coarse bands: ground / dynamic /
  UI), secondary key world Y position (via a `sortOriginY` offset,
  defaulting to `Transform.y`) for entities sharing a layer. This is
  what lets the player walk behind a tree when above it and in front of
  it when below — draw order flips naturally as Y position crosses.
- **Trigger/sensor system** — Box2D sensor fixtures (`isSensor = true`,
  no physical collision response, overlap-only). Box2D 3.x has no
  contact-listener class to subclass (see README.md's Box2D version
  note) — sensor overlap events are buffered internally during
  `b2World_Step` and read back afterward via `b2World_GetSensorEvents()`,
  which is what `PhysicsWorld::step()` does, pushing them into a
  `TriggerEvent` queue (never doing gameplay logic inside the physics
  step itself — Box2D disallows body creation/destruction mid-step, and
  prompt/UI logic can easily do that indirectly). `PhysicsWorld::
  drainTriggerEvents()` drains the queue afterward. An `Interactable` component
  (`{ promptText }`) attached to a sensor-bearing entity is what
  Milestones 6 and 7 use for "press E to loot" / "press E to enter
  vehicle" prompts.

**Acceptance:** the player and a test dynamic body collide correctly
against tilemap walls via Box2D (no more hand-rolled collision code in
`main.cpp`); a rotated test object correctly occludes/is-occluded-by the
player as they cross its Y position; walking into a sensor zone around a
test object fires an enter event and shows a placeholder prompt, walking
back out fires an exit event and clears it.

---

## Milestone 5 — Combat & movement feel

**Goal:** the first milestone where this stops feeling like a tech demo.

- **Aim direction** — mouse-relative or right-stick, independent of
  movement direction (classic twin-stick-adjacent top-down aiming), now
  expressed as a GLM `vec2` rather than raw doubles.
- **Weapon component + hitscan firing** — `Weapon{fireRate, damage,
  range, spread}`; firing does a ray/segment test against enemy
  colliders — a Box2D raycast (`b2World::RayCast`), not a hand-rolled
  segment test, now that the physics world exists. Projectile-based
  weapons (travel-time, ballistic drop) are a later, separate weapon
  *type* on the same component — not a rewrite.
- **`Health` component** + damage application, death → despawn (or
  corpse entity, feeding Milestone 6 loot).
- **Recoil / screen shake / muzzle flash placeholder** — cheap juice
  that makes shooting feel like it's doing something, using the color-
  tint/scale tricks already available from `Sprite` — no new rendering
  pipeline needed yet.
- **Basic hostile stub** — a static or simple-patrol target dummy entity
  with `Health`, purely to have something to shoot and confirm damage
  actually lands. Real AI is Milestone 8.

**Acceptance:** you can aim independently of movement, fire, see a hit
register on a target dummy, and feel a screen-shake/flash on impact.

---

## Milestone 6 — Inventory & loot

**Goal:** the Zero-Sievert-style grid inventory you already prototyped in
GameMaker, rebuilt properly in C++ on top of the real ECS.

- **`engine::ui`** — minimal immediate-mode-ish widget layer: panels,
  grid slots, drag-and-drop, hover/tooltip. Generic UI plumbing lives in
  `engine/`; the specific inventory *layout* (backpack/equipment/
  container regions) lives in `game/`.
- **Item database** — data-driven item definitions (id, name, icon,
  stack size, weight, category) loaded from JSON via `nlohmann::json`,
  not hardcoded in C++, so adding items doesn't require a recompile-and-
  redesign cycle.
- **`Inventory` component** — grid-based slot storage, weight/stack
  limits.
- **Loot containers & corpse looting** — a `Lootable` component
  attached to containers and dead enemies, using the Milestone 4 trigger
  system's `Interactable` prompt to open a shared inventory-transfer UI.

**Acceptance:** kill the Milestone 5 target dummy, it drops a lootable
corpse, get a "press E to loot" prompt near it (trigger system), open it,
drag items into your inventory, see weight/stack limits enforced.

---

## Milestone 7 — Vehicles

**Goal:** rideable, fuel-limited, repairable vehicles — the system this
roadmap update was written for.

- **`Drivable` component** (`maxSpeed, acceleration, turnRate, drag`) on
  vehicle entities, each with their own `RigidBody` (dynamic Box2D body,
  rotation enabled — unlike the player body).
- **Top-down car physics** — the standard Box2D pattern: forward force
  along the body's current facing angle scaled by throttle input,
  angular velocity for steering scaled by current speed (no turning in
  place at a standstill — that's what makes it read as a car, not a
  twin-stick character), and a lateral friction impulse that kills
  sideways velocity so the vehicle doesn't slide like it's on ice.
  Tuning these three forces is where "feels like a real vehicle" comes
  from.
- **`Fuel` and `Durability` components** — `Fuel` drains while the
  vehicle is driven and possessed; at zero, throttle force drops to zero
  (dead in the water, not deleted). `Durability` drops on collision
  impact (via Box2D contact events) and is restored by using a repair
  item (Milestone 6 inventory) while near the vehicle.
- **Possession pattern** — app-level `Entity possessed` state, starting
  as the player. Movement/input systems branch on *what components the
  possessed entity has* (`has<Drivable>()`) rather than on entity
  identity. Entering a vehicle (via its Milestone 4 sensor/prompt):
  the player entity is never destroyed — its physics body is disabled/
  filtered out of collision and it stops rendering, while `possessed`
  and camera-follow redirect to the vehicle. Exiting reverses this and
  places the player at a validated (non-wall-overlapping) point near the
  vehicle.

**Acceptance:** walk up to a vehicle, get a prompt, enter it, drive with
real momentum/steering/friction against the tilemap's Box2D walls,
watch fuel drain and stop the vehicle at zero, take damage on collision,
repair it with an item, exit back to on-foot control at a valid position.

---

## Milestone 8 — AI

**Goal:** enemies that behave like a threat, not a target dummy.

- **State machine** (`idle → patrol → investigate → chase → attack`) as
  an `AIState` component + `AISystem`, driven by simple perception
  (distance + line-of-sight check via Box2D raycast against static
  geometry).
- **Grid-based pathfinding** (A* over a coarse tile grid derived from
  the tilemap's static collision bodies) for chase/patrol movement that
  actually routes around walls instead of beelining through them.
- **Enemy archetypes** — data-driven stats (health, speed, damage,
  detection range) so "raider" vs "mutant" is a data difference, not a
  code fork.
- **Spawn system** — zone-based or trigger-based enemy spawning (reusing
  the Milestone 4 trigger system).

**Acceptance:** an enemy on patrol notices the player within range/LOS,
paths toward them around obstacles, and attacks in melee or ranged
fashion depending on archetype.

---

## Milestone 9 — Survival stats

**Goal:** the systemic pressure that defines the genre — you're not just
fighting enemies, you're fighting your own body.

- **Stat components** — `Hunger`, `Thirst`, `Radiation`, `Bleeding`, each
  ticking over time via a `SurvivalSystem`, with thresholds that trigger
  status effects (movement penalty, damage-over-time, screen vignette).
- **Consumables** — food/water/anti-rad items (from the Milestone 6 item
  database) that modify these stats on use.
- **Stat UI** — HUD bars built on the Milestone 6 UI widgets.

**Acceptance:** stats visibly drain over real playtime, penalties kick in
at low values, and consuming the right item recovers the right stat.

---

## Milestone 10 — World systems: day/night, zone streaming, weather

**Goal:** the world feels alive and larger than one loaded map.

- **Day/night cycle** — a global time-of-day value driving ambient
  lighting (color-mod overlay to start; a proper lighting pass is an
  optional later polish item) and gameplay hooks (enemy spawn rates,
  visibility range).
- **Zone/chunk streaming** — load/unload adjacent `TileMap` regions (and
  their Box2D static bodies) as the player crosses zone boundaries, so
  the explorable world isn't capped at one map's tile grid.
- **Weather** (optional stretch within this milestone) — simple particle
  overlays (rain/dust) plus a gameplay hook (reduced visibility).

**Acceptance:** walking far enough in one direction streams in a new
zone seamlessly; day/night visibly cycles over a play session.

---

## Milestone 11 — Save / load

**Goal:** persistence — the point where a play session becomes a
character/save-file, not a one-off run.

- **ECS serialization** — walk all registered component pools, serialize
  to JSON (`nlohmann::json`) keyed by entity, restore on load rebuilding
  entities/components (not raw handles, which aren't stable across
  runs) and their Box2D bodies.
- **Save slots** — simple file-based save/load with a save-select screen
  (built on Milestone 6's UI widgets).

**Acceptance:** save mid-session, close the game, relaunch, load — player
position, inventory, survival stats, vehicle state, and world/zone state
all restore.

---

## Milestone 12 — Audio & polish pass

**Goal:** the "juice" pass — this is where a technically-complete game
starts *feeling* good to play, which is its own real engineering work,
not an afterthought.

- **`engine::audio`** — `SDL_mixer` wrapper: SFX playback with distance-
  based volume/panning, music with crossfade.
- **Juice pass** — hit-flash, damage numbers, particle system (muzzle
  flash, blood, dust kicked up by footsteps and vehicle tires), screen
  shake tuning, squash/stretch on impacts. This is exactly the kind of
  polish you've called out before as something you care about — it's
  scheduled last on purpose, once there's a full game loop worth
  polishing rather than polishing systems that might still change shape.

**Acceptance:** the game has real audio feedback for every major action
(shoot, hit, loot, engine start/stop, low-health/low-fuel warning) and
visual impact feedback that makes combat and driving feel responsive
rather than numbers changing silently.

---

## Cross-cutting principles (apply at every milestone)

- **Framework/game split is enforced, not aspirational.** If a new piece
  of code has no Ashworld/Zero-Sievert-specific meaning, it goes in
  `engine/`, full stop — even if "just this once" it'd be faster to
  hardcode it in `game/`.
- **Data over code where it matters.** Items, enemy archetypes, vehicle
  stats, and (eventually) weapon stats live in JSON, not C++ structs
  baked into the binary — because tuning a survival/loot/vehicle game is
  mostly *data* tuning, and that shouldn't require a recompile.
- **Every milestone ships something runnable.** No milestone is "done"
  until it's been built and run (headless-verified here, real-window-
  verified on your end) — not just written and assumed correct.
- **Minimal dependencies, chosen deliberately.** Current/planned
  dependency list:
  - SDL2, SDL2_image, tinyxml2 — Milestone 1/3 (window, tileset art, TMX
    parsing)
  - **GLM, Box2D — Milestone 4** (vector math, rigid-body physics/
    collision — replaces the hand-rolled AABB approach from Milestone 3)
  - `nlohmann::json` — Milestone 6 (item database), reused at Milestone
    11 (save/load)
  - `SDL_mixer` — Milestone 12 (audio)
  Nothing gets added without a milestone that actually needs it.
