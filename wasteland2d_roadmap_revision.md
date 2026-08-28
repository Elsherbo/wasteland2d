# ROADMAP.md revision — Milestone 5.5, and resequencing 6 & 12

Three edits, all to `ROADMAP.md`. Written in the document's existing voice/
format (goal statement, bulleted features with reasoning, an Acceptance
line) so it drops in without a style mismatch.

1. **Insert a new Milestone 5.5**, between the current Milestone 5 and
   Milestone 6 sections.
2. **Add one bullet to Milestone 6** noting where multi-weapon loadout
   actually belongs.
3. **Trim Milestone 12** so it no longer duplicates infrastructure that
   Milestone 5.5 now owns — it keeps only genuinely new *content*.

No renumbering of Milestones 6–12 — "5.5" avoids a disruptive full
renumber for what's an insertion, not a restructure.

---

## 1. Insert after Milestone 5, before Milestone 6

```markdown
## Milestone 5.5 — Real sprites, attachment, camera & cursor polish, juice infrastructure

**Goal:** close the gap between "colored rectangles" and an actual
character on screen, and stop hand-writing one-off timer/effect code
every time a milestone wants visual feedback. This milestone was added
after Milestone 5 shipped and made the gap concrete: multi-weapon
loadouts, layered character sprites, and a weapon that visibly tracks
the mouse are all blocked on pieces of engine infrastructure that were
implied but never actually scoped. This milestone is that infrastructure
— no new gameplay systems, everything here is a `engine/` primitive that
Milestone 6 onward builds on.

- **Real texture rendering** — `TextureCache` gains the `load(path) ->
  SDL_Texture*` method its own Milestone 2 comment already promised
  ("the API shape is designed to drop in without touching call sites
  that already use `whitePixel()`"), backed by SDL2_image — already a
  project dependency since Milestone 3's tileset loading, so this adds
  **zero new dependencies**. `TileMap`'s tileset image loading is
  repointed at this same cache instead of loading its own texture
  independently, so there's one texture-owning path in the engine, not
  two. `Sprite` gains an optional texture handle + source rect (for
  spritesheet/atlas frames); `width`/`height`/`color` stay as-is and
  keep working exactly as before for anything that doesn't set a
  texture — colored-rectangle placeholders (like the Milestone 5 target
  dummy) aren't going away, they're just no longer the *only* option.

- **`Attachment` component + `AttachmentSystem`** — the actual missing
  primitive underneath both "layered character sprites" and "weapon
  rotates independently of the body." `Attachment{ parent: Entity,
  localOffset: glm::vec2, followRotation: bool }`; `AttachmentSystem`
  runs once per fixed update, after physics sync, copying each attached
  entity's `Transform` position from its parent's `Transform` (offset
  by `localOffset`, rotated with the parent if `followRotation` is
  true). This is deliberately entity-per-layer, not a
  `CompositeSprite{ vector<Layer> }` component bundling several sprites
  into one entity — the sparse-set ECS already enforces one `Sprite`
  per entity, and entity-per-layer means Y-sorting, individual layer
  visibility, and per-layer texture swaps (different hair, different
  pants) all fall out of systems that already exist instead of needing
  their own special cases. A player becomes one root entity (`Transform`
  + movement + physics body) plus N attached entities (body, hair,
  pants, equipped weapon, ...), each a normal `Transform`+`Sprite`
  entity that `SpriteRenderSystem` already knows how to draw.

- **Weapon rotation toward aim** — the equipped-weapon entity is an
  `Attachment` with `followRotation = false`; its `Transform.
  rotationDegrees` is set every frame from the same aim-direction vector
  `CombatSystem::fireWeapon()` already consumes (Milestone 5) — this is
  a rendering-only addition on top of aim math that already exists, not
  a new aim system. (Verify against the Milestone 4 audit note on Box2D
  rotation convention lining up with SDL's clockwise convention *because*
  this project is Y-down throughout — the same reasoning applies here.)

- **Camera aim-offset** — a sibling to `Camera::followSmooth`: the
  follow target is biased toward the aim direction (`playerPos + aimDir
  * maxOffsetPixels`), smoothed at its own (likely slower) rate so quick
  aim flicks don't jitter the camera. Small, self-contained addition to
  `Camera`, same shape as the shake-offset addition from Milestone 5.

- **Custom cursor capability** — hide the OS cursor and draw a texture
  (via the same `TextureCache`) at the mouse position instead, with a
  `setCursor(textureId)` / `resetCursor()` API. This milestone only
  builds the *capability* — deciding *when* to swap cursors (default vs.
  weapon-specific vs. inventory-open) needs a real trigger condition to
  hook into, and the first one of those (inventory open/closed) doesn't
  exist until Milestone 6's UI system does. That policy belongs there,
  not here.

- **Generic juice/effects infrastructure** — Milestone 5 shipped three
  independent, hand-rolled timer/decay blocks in `main.cpp`
  (`muzzleFlashTimer`, `shakeTimer`/`shakeMagnitude`,
  `tracerStart`/`tracerEnd`/`tracerTimer`), each with its own copy-pasted
  decay logic. This milestone generalizes that pattern instead of
  writing it a fourth, fifth, and sixth time as more effects show up:
  a small `engine::fx` module providing (1) a **flash** effect —
  temporarily overrides a `Sprite`'s color, reverting after a duration,
  replacing the hand-written muzzle-flash/damage-darken color-swap code
  from Milestone 5; (2) **screen shake** — the decay math `main.cpp`
  currently hand-rolls around `Camera::setShakeOffset`, formalized into
  one shared helper; (3) a fire-and-forget **world-space line/marker**
  effect, generalizing the Milestone 5 tracer. This is infrastructure
  only — it's a refactor of Milestone 5's existing effects onto a shared
  primitive, not new visual content. New effect *types* (particles,
  damage numbers, squash/stretch) stay in Milestone 12, where they've
  always been scoped, now built on top of this instead of from scratch.

**Acceptance:** the player renders as several layered sprites (body +
hair + pants placeholders) that move together as one unit; the equipped
weapon is a separate attached sprite that visibly rotates to track the
mouse independent of body/movement direction; the camera visibly (but
subtly) leans toward the aim direction; a custom cursor texture renders
in place of the OS cursor; and Milestone 5's muzzle-flash, screen-shake,
and tracer are now implemented through the shared `engine::fx`
primitive instead of hand-rolled `main.cpp` timers — with **no visible
behavior change** from Milestone 5 (this is a refactor validated by
looking identical, not by looking new).
```

---

## 2. Add to Milestone 6 (Inventory & loot)

Insert as a new bullet, after the existing `Inventory` component bullet
and before "Loot containers & corpse looting":

```markdown
- **Equipment slots & multi-weapon loadout** — `primary`/`secondary`/
  `melee` weapon slots on the player, each holding an item-database
  reference (previous bullet) rather than a single hardcoded `Weapon`
  component instance (Milestone 5's scope). Switching the active slot
  swaps which `Weapon` data `CombatSystem` reads and which attached
  weapon sprite (Milestone 5.5's `Attachment`) is visible — a data swap
  driven by inventory/equipment state, not a rewrite of either system.
  This is deliberately sequenced here rather than bolted onto Milestone
  5's single-`Weapon` player setup, since "data over code where it
  matters" (this document's own cross-cutting principle) means weapon
  loadout should be built on real equipment slots the first time, not
  hardcoded now and migrated later. The Milestone 5.5 custom-cursor
  capability's first real caller also lands here — default cursor while
  the inventory panel is open, weapon-specific cursor otherwise.
```

---

## 3. Trim Milestone 12 (Audio & polish pass)

**Current** "Juice pass" bullet:
```markdown
- **Juice pass** — hit-flash, damage numbers, particle system (muzzle
  flash, blood, dust kicked up by footsteps and vehicle tires), screen
  shake tuning, squash/stretch on impacts. This is exactly the kind of
  polish you've called out before as something you care about — it's
  scheduled last on purpose, once there's a full game loop worth
  polishing rather than polishing systems that might still change shape.
```

**Replace with:**
```markdown
- **Juice content pass** — built on Milestone 5.5's `engine::fx`
  infrastructure (flash/shake/marker effects already exist by this
  point, from Milestone 5): new effect *types* — damage numbers, a
  particle system (muzzle flash, blood, dust kicked up by footsteps and
  vehicle tires), squash/stretch on impacts — plus final tuning of
  every effect against the finished game loop. This is exactly the kind
  of polish you've called out before as something you care about — it's
  scheduled last on purpose, once there's a full game loop worth
  polishing rather than polishing systems that might still change
  shape. (The *infrastructure* moved earlier, to Milestone 5.5 — it was
  already being duplicated by hand in Milestone 5's combat feedback, so
  waiting until here to generalize it would mean writing it three
  separate times first.)
```

---

That's the full revision. Once this is in, the sequencing from here reads:
Milestone 5 (combat, done) → **5.5 (this)** → Milestone 6 (inventory, now
where multi-weapon loadout actually belongs) → 7 (vehicles) → ... → 12
(content-only juice pass, no longer duplicating 5.5's infra).
