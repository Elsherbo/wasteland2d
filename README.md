# wasteland2d

A reusable C++20 2D top-down game framework, built alongside a first game
in the **Ashworld** / **Zero Sievert** tradition — irradiated-wasteland
survival, scavenging, tension-driven top-down combat.

The project is split into two CMake targets from day one, and that split
is enforced throughout development, not just at the start:

| Target      | Type        | Contains |
|-------------|-------------|----------|
| `engine`    | static lib  | The reusable framework — window/loop, ECS, rendering, tilemaps, physics. Nothing in here knows what "radiation" or "loot" means. |
| `wasteland2d` | executable | This specific game, built entirely on top of `engine`. |

Your next top-down game reuses `engine/` unchanged and only replaces
`game/`.

**Full build plan:** see [`ROADMAP.md`](./ROADMAP.md) for all 10
milestones, what each one delivers, and the acceptance criteria used to
call it done.

---

## Status

| Milestone | Description | Status |
|---|---|---|
| 1 | Window, fixed-timestep loop, action-mapped input | ✅ Complete |
| 2 | Sparse-set ECS, sprite rendering, camera | ✅ Complete |
| 3 | Tiled `.tmx` maps, AABB collision | ✅ Complete |
| 4 | Physics foundation: GLM, Box2D, Y-sorting, triggers | ✅ Complete |
| 5 | Combat & movement feel | ⏳ Next |
| 6 | Inventory & loot | Planned |
| 7 | Vehicles (rideable, fuel, repair) | Planned |
| 8 | AI (state machine + pathfinding) | Planned |
| 9 | Survival stats (hunger/thirst/radiation) | Planned |
| 10 | Day/night, zone streaming, weather | Planned |
| 11 | Save / load | Planned |
| 12 | Audio & polish pass | Planned |

Current build: a player-controlled entity, driven by a real Box2D
dynamic body, moves through a real Tiled map whose collision rectangles
are now Box2D static geometry — no more hand-rolled AABB checks.
Sprites Y-sort within their layer (walk behind/in front of tall
objects), and a sensor-based trigger system fires enter/exit events with
an `Interactable` prompt, demoed against a static vehicle placeholder.
Milestone 5 builds real combat on top of this: hitscan via
`PhysicsWorld::raycast`, a `Health` component, and a target dummy to
shoot.

---

## Architecture

### Engine (`engine/`) — reusable framework

```
engine/
  core/       Application (fixed-timestep loop + callbacks), Window, Clock
  input/      InputManager — action-mapped, rebindable (Action enum, not raw scancodes)
  ecs/        Registry, ComponentPool<T>, Entity (index+generation handle),
              Components.h (Transform)
  render/     Camera (world<->screen, smooth-follow), Sprite component,
              SpriteRenderSystem (layer-sorted draw pass)
  physics/    AABB, Collider component
  world/      TileMap — Tiled .tmx loader (embedded tileset, CSV layers,
              "collision" object layer)
  resource/   TextureCache (currently: shared white-pixel texture for
              tinted placeholder sprites; real image loading lands here
              in Milestone 4+ as art comes in)
  ui/         (Milestone 5 — grid inventory, drag/drop)
  audio/      (Milestone 10 — SDL_mixer wrapper)
```

### Game (`game/`) — this project, replace per future project

```
game/
  main.cpp      Wires Application callbacks; currently the whole "game" —
                 loads the map, owns the player entity, does input->movement
  components/    (Milestone 4+ — Health, Weapon, Inventory, AIState, ...)
  systems/       (Milestone 4+ — CombatSystem, LootSystem, AISystem, ...)
  entities/      (Milestone 4+ — prefab/factory functions)
```

### Assets

```
assets/
  tilesets/   placeholder_tileset.png — 2-tile placeholder art (floor, wall)
  maps/       sample_map.tmx — 20x15 test map (border walls + interior block)
```

`assets/` is copied next to the built binary automatically as a
post-build step (see `CMakeLists.txt`).

### Design principle: the framework test

Every time new code is written, it's asked one question: **could this
ship unchanged in a totally different top-down game?** Yes → `engine/`.
No → `game/`. This is what keeps `engine/` an actual reusable framework
instead of slowly becoming "the wasteland game with extra folders." See
`ROADMAP.md` → *Cross-cutting principles* for the full list of rules
applied at every milestone (data-driven content, dependency discipline,
"every milestone ships something runnable").

---

## Dependencies

| Library | Purpose | Joins at |
|---|---|---|
| [SDL2](https://www.libsdl.org/) | Window, renderer, input | Milestone 1 |
| [SDL2_image](https://github.com/libsdl-org/SDL_image) | Tileset PNG loading | Milestone 3 |
| [tinyxml2](https://github.com/leethomason/tinyxml2) | Tiled `.tmx` (XML) parsing | Milestone 3 |
| [GLM](https://github.com/g-truc/glm) | Vector/matrix math (movement, aim, steering) | Milestone 4 |
| [Box2D](https://box2d.org/) (3.x) | Rigid-body physics: collision, rotation, vehicle steering, trigger/sensor zones | Milestone 4 |
| `nlohmann::json` | Item database, save files | Milestone 6/11 *(not yet added)* |
| SDL_mixer | Audio | Milestone 12 *(not yet added)* |

Nothing is added to this list without a milestone that actually needs
it — see the dependency-discipline principle in `ROADMAP.md`. Box2D
replaced the hand-rolled AABB collision built in Milestone 3 — see
`ROADMAP.md` → Milestone 4 for why, and `engine/physics/PhysicsWorld.h`
for the wrapper boundary (game code never includes `<box2d/box2d.h>` or
calls a `b2*` function directly — only `engine::physics::PhysicsWorld`'s
engine-shaped API).

**Box2D version note:** Box2D 3.0 was a complete rewrite — the old
pointer-based C++ classes (`b2World`, `b2Body`, `b2Fixture`,
`b2ContactListener`) were replaced with an opaque-ID, plain-C API
(`b2WorldId`, `b2BodyId`, `b2CreateWorld`, `b2Body_GetPosition`, ...).
vcpkg's `box2d` port currently defaults to the 3.x line (3.1.1 as of
this writing) — `PhysicsWorld.cpp` targets that API. If a future vcpkg
update ever jumps to a 4.x with another breaking rewrite, that's the
file to revisit; nothing in `game/` would need to change, since it never
touches Box2D types directly.

---

## Building on Windows (MSVC + vcpkg)

This assumes you're already set up with CMake + MSVC, per your usual
workflow.

1. Install [vcpkg](https://github.com/microsoft/vcpkg) if you don't have
   it:
   ```powershell
   git clone https://github.com/microsoft/vcpkg
   .\vcpkg\bootstrap-vcpkg.bat
   ```
2. Install dependencies through it:
   ```powershell
   .\vcpkg\vcpkg install sdl2:x64-windows sdl2-image:x64-windows tinyxml2:x64-windows glm:x64-windows box2d:x64-windows
   ```
3. Configure and build, pointing CMake at vcpkg's toolchain file:
   ```powershell
   cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
       -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Debug
   ```
   (Note: in PowerShell, the line-continuation character is a backtick
   `` ` ``, not `^` — `^` is CMD's continuation character. Use whichever
   matches the shell you're actually running in, or put it all on one
   line.)
4. Run **from the project root**, not from inside `build/`, so the
   relative `assets/maps/...` path in `main.cpp` resolves correctly:
   ```powershell
   .\build\Debug\wasteland2d.exe
   ```

If you'd rather not use vcpkg, `find_package(SDL2 CONFIG REQUIRED)` (and
the `SDL2_image`/`tinyxml2` equivalents) also work with manually-built
dev packages, as long as the relevant `*_DIR` CMake variables point at
the folders containing their `*Config.cmake` files.

## Building on Linux (for reference / CI)

```bash
sudo apt install cmake libsdl2-dev libsdl2-image-dev libtinyxml2-dev libglm-dev libbox2d-dev
cmake -B build -S .
cmake --build build -j
./build/wasteland2d
```

Every milestone in this repo is built and run this way before being
handed off, specifically to catch compile errors, TMX-parsing bugs, and
runtime crashes before you ever see them — the Windows/vcpkg path is
untested on my end since only Linux is available here, so flag it
immediately if `find_package(... CONFIG)` doesn't pick up your vcpkg
install cleanly.

---

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| `Shift` (held) | Sprint (1.6x speed) |
| `Esc` | Quit |

---

## Project conventions

- **C++20**, built with `/W4` (MSVC) or `-Wall -Wextra -Wpedantic`
  (GCC/Clang) — zero warnings is the bar, not a suggestion.
- **Namespaces mirror folders**: `engine::ecs`, `engine::render`,
  `engine::world`, `engine::physics`, etc. — a `using` you write for one
  file tells you exactly where the header lives.
- **Headers own documentation.** Design decisions and deliberate scope
  limits (e.g. `TileMap`'s "CSV-only, embedded-tileset-only" constraint)
  are documented as comments directly above the relevant class, not in a
  separate doc that can drift out of sync with the code.
- **Every component is a plain struct.** No inheritance hierarchies, no
  virtual dispatch in hot paths — systems are free functions or static
  methods operating on `Registry::view<Ts...>()` results.

---

## Roadmap

See [`ROADMAP.md`](./ROADMAP.md) for the complete milestone-by-milestone
plan (Milestones 4 through 10), including scope, technical approach, and
acceptance criteria for each.
