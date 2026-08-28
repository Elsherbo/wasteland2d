// Must come before <SDL.h> — otherwise SDL redefines main() to SDL_main()
// on Windows, and a plain console-subsystem exe (which is what we build)
// can't find a "main" symbol to link against. We want the console window
// anyway, for debug logging during development.
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cmath>
#include <cstdio>
#include <optional>
#include <tuple>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "core/Application.h"
#include "ecs/Attachment.h"
#include "ecs/AttachmentSystem.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"
#include "fx/FlashEffect.h"
#include "fx/ScreenShake.h"
#include "fx/WorldLineEffects.h"
#include "input/Cursor.h"
#include "physics/Interactable.h"
#include "physics/InteractionTracker.h"
#include "physics/PhysicsSyncSystem.h"
#include "physics/PhysicsWorld.h"
#include "render/Camera.h"
#include "render/Color.h"
#include "render/Sprite.h"
#include "render/SpriteRenderSystem.h"
#include "resource/TextureCache.h"
#include "ui/GridLayout.h"
#include "ui/GridRenderer.h"
#include "render/Font.h"
#include "render/TextRenderer.h"
#include "world/TileMap.h"

#include "components/EquipmentSlots.h"
#include "components/Health.h"
#include "components/Inventory.h"
#include "components/Lootable.h"
#include "components/LootDrop.h"
#include "components/MeleeWeapon.h"
#include "components/QuickSlots.h"
#include "components/Weapon.h"
#include "data/ItemDatabase.h"
#include "systems/CombatSystem.h"
#include "systems/Encumbrance.h"
#include "systems/EquipmentSystem.h"
#include "systems/InventorySystem.h"
#include "systems/MeleeCombatSystem.h"
#include "systems/UseItemSystem.h"
#include "ui/DragDropController.h"
#include "ui/EquipmentRenderer.h"
#include "ui/InventoryRenderer.h"

// Milestone 4: the hand-rolled AABB collision from Milestone 3 is gone —
// tilemap walls and the player are now real Box2D bodies, stepped by
// engine::physics::PhysicsWorld. Two new things are demoed here that
// don't affect movement at all, purely to prove they work:
//   - a tall "tree" entity to show Y-sorted draw order (walk above it,
//     then below it, and watch the player go from being drawn on top to
//     being drawn behind it)
//   - a static "vehicle placeholder" with a sensor fixture + an
//     Interactable, to prove the trigger system fires enter/exit events
//     (watch the console, and the placeholder's color brightens while
//     you're in range) — full drivable vehicles are Milestone 7.
//
// Milestone 5 adds combat & movement feel on top of that:
//   - player movement is exponentially smoothed toward the input
//     direction (accelerate/decelerate) instead of an instant velocity
//     snap, for a touch of weight without becoming a sliding physics
//     object — see playerAccel/playerDecel below.
//   - mouse-aim, independent of movement direction, converted from
//     screen space to world space via Camera::screenToWorld.
//   - a Weapon on the player fires a hitscan raycast (via
//     PhysicsWorld::raycast, same one Milestone 4 added), on cooldown
//     per its fireRate — see game::systems::CombatSystem.
//   - a static target dummy with Health to shoot; taking damage darkens
//     its sprite (cheap, no health-bar UI yet — that's Milestone 6),
//     and death despawns it.
//
// Milestone 5.5 closes the gap between "colored rectangles" and an
// actual character on screen, and generalizes Milestone 5's one-off
// combat-feedback code into reusable engine/ primitives:
//   - TextureCache::load() + Sprite::texture/sourceRect add real
//     texture rendering (still unused by any entity below — every
//     entity here still deliberately uses the colored-rectangle
//     placeholder path, since real art is a Milestone 6+ content
//     concern; this milestone is the *capability*, not the art).
//   - ecs::Attachment + AttachmentSystem — a generic "this entity's
//     Transform follows another entity's" primitive. The player is now
//     one root entity plus three attached entities (hair, pants,
//     weapon) — proving the mechanism, not final character art.
//   - the equipped weapon is one of those attachments, with
//     followRotation=false — its rotation is set every frame from the
//     same aimDir CombatSystem already consumes, so it visibly tracks
//     the mouse independent of movement.
//   - Camera::followSmoothWithOffset/zoomSmooth add a camera lean +
//     zoom toward the aim direction — see the ADS (aim-down-sights)
//     update below.
//   - ADS: holding right-click zooms in and leans the camera toward
//     the aim direction, scaled per-weapon via
//     Weapon::adsZoomMultiplier/adsOffsetPixels (a pistol barely
//     moves; an AR/sniper would lean/zoom further once Milestone 6's
//     equipment slots let more than one Weapon exist at once).
//   - engine::input::Cursor adds a custom-cursor capability (demoed
//     here as a plain tinted square via TextureCache::whitePixel(),
//     same placeholder pattern as everything else — real cursor art,
//     and any policy around *when* to swap cursors, is Milestone 6+).
//   - engine::fx (FlashEffect/FlashSystem, ScreenShake,
//     WorldLineEffects) replaces Milestone 5's three hand-rolled timer
//     blocks (muzzleFlashTimer, shakeTimer/shakeMagnitude,
//     tracerStart/tracerEnd/tracerTimer) with shared, reusable
//     primitives — behavior is unchanged from Milestone 5.
//
// Milestone 6 (data layer only — see the delivery notes) adds
// inventory & loot:
//   - game::data::ItemDatabase loads assets/items/items.json into a
//     lookup table (nlohmann::json, isolated to ItemDatabase.cpp —
//     same pattern PhysicsWorld hides Box2D and TileMap hides
//     tinyxml2 behind their own headers).
//   - game::components::Inventory is a Tarkov/Zero-Sievert-style
//     shaped grid (items occupy a width x height footprint, not one
//     item per slot); game::systems::InventorySystem owns all
//     placement/overlap/stacking/weight-limit logic. The player has
//     one; see InventorySystem's own header for the full contract.
//   - CombatSystem's death handling (Milestone 5 despawn-only) now
//     spawns a walkable, lootable corpse for any dying entity that
//     carries a LootDrop component — the target dummy has one. A dying
//     entity without LootDrop still just despawns, unchanged.
//   - Looting itself (pressing E near a corpse) is wired through
//     InventorySystem::moveAllTo and printed to the console, AND (new
//     in this pass) a real drag-and-drop grid UI — press Tab to open
//     the player's inventory panel; a second panel for whatever
//     Lootable is nearby appears alongside it. engine::ui::GridLayout/
//     GridRenderer are the generic (screen<->cell math, cell
//     background rendering) engine-level pieces; game::ui::
//     DragDropController and InventoryRenderer are game-level, since
//     they reference Inventory/ItemDatabase directly — same
//     engine/game split reasoning as InventorySystem. This also closes
//     a gap flagged back in Milestone 5.5: opening the inventory now
//     swaps to the OS cursor (precise UI interaction) and swaps back
//     to the custom aim cursor on close — the real trigger condition
//     that didn't exist yet at the time.
//   - Player movement/aim/fire/ADS are all suspended while the
//     inventory is open (a menu-open pause, same genre convention as
//     Zero Sievert/Tarkov) rather than usable underneath the panel.
//
// Added after in-game feedback on the drag-and-drop UI:
//   - Rotation (R while dragging) — InventoryStack::rotated, toggled
//     via DragDropController::toggleRotation. Manual/drag-time only;
//     automatic placement (looting, corpse spawning) never rotates
//     anything on its own — see Inventory.h.
//   - Swapping — dropping a held item onto exactly one other stack
//     whose footprint exactly matches trades their positions, instead
//     of just rejecting the drop. Same-inventory (reorganizing) and
//     cross-inventory (player <-> a lootable) both work.
//   - Quick-grab/quick-store — Ctrl+click a stack to instantly send it
//     to "the other" visible panel, without a drag. Reuses
//     InventorySystem::quickTransferStack — same partial-transfer,
//     nothing-lost behavior as looting everything.
//   - Hover highlighting and drop-preview (green/red) — the preview
//     reads DragDropController::previewDrop(), which is the exact same
//     decision endDrag() itself acts on (see DragDropController.h's
//     planDrop()), so the preview can never show something different
//     from what actually happens on release.
//   - Grab-point-relative dragging — picking up a shaped item no
//     longer snaps its top-left corner under the cursor; the drag
//     ghost tracks the cell you actually clicked (HeldStack::
//     grabOffsetX/Y), and DragDropController::resolveDropTopLeft()
//     converts "mouse is over this cell" into "the item's top-left
//     would land here" for both the highlight preview and the actual
//     drop — one function, used by both, so they can't disagree.
//
// Real text rendering, closing the gap flagged above three separate
// times: engine::render::Font (RAII TTF_Font wrapper) + TextRenderer
// (rasterizes via SDL_ttf, caches the resulting texture per (text,
// color) pair so drawing the same string repeated frames doesn't
// re-rasterize it every time — see TextRenderer.h's own note on why
// that cache is deliberately unbounded for now, and when it would stop
// being reasonable). TTF_Init()/TTF_Quit() now run alongside
// SDL_Init/SDL_Quit in Application.cpp — and IMG_Init()/IMG_Quit() got
// added there too, a small found-and-fixed bonus: texture loading had
// been relying on SDL2_image's undocumented lazy auto-init the whole
// time, which happened to work but was never guaranteed to.
// First real uses, closing three specific gaps this file had been
// flagging: stack quantities (a "x30" in the corner of a cell),
// a used/max weight readout on the player's panel, and a plain
// hover tooltip showing an item's name. All render via one bundled
// font — assets/fonts/VT323-Regular.ttf (SIL OFL licensed, see
// assets/fonts/OFL.txt) — a terminal/typewriter face that fits the
// wasteland setting and reads fine even small.
//
// Equipment slots — the original ask that kicked off everything from
// Milestone 5.5 onward, now unblocked by real inventory existing:
//   - data::WeaponStats (ItemDefinition.h) carries combat stats on
//     weapon items — ranged (fireRate/damage/range/spread/ADS) or
//     melee (damage/range/arc/attacksPerSecond), tagged by
//     WeaponKind. pistol/smg are ranged; combat_knife is melee.
//   - MeleeCombatSystem is a genuinely separate mechanic from
//     CombatSystem (a swing/arc hit-check — "what's in this area",
//     not "what's the first thing along this ray") — see its own
//     header for why. The two share only Damage::apply() (Damage.h,
//     extracted from CombatSystem this pass) — "hit something, maybe
//     kill it, maybe spawn a corpse" is identical either way, and
//     duplicating that logic a second time risked reintroducing the
//     exact leaked-physics-body bug it was first written to fix.
//   - EquipmentSystem::equip()/unequip() move an item between the
//     inventory grid and a slot (loadout management — done via the
//     inventory UI: press 1/2/3 while hovering a compatible item, not
//     mid-drag); syncActiveWeapon() copies whichever slot is active
//     onto the player's Weapon or MeleeWeapon component (removing the
//     other), so CombatSystem/MeleeCombatSystem's existing "does this
//     entity have the component" gate is the only thing that needs to
//     know what's currently usable.
//   - Switching (which already-equipped slot is in-hand) is a
//     completely different action from equipping, and only usable
//     with the inventory closed: both 1/2/3 and the scroll wheel work,
//     per your call — mouse wheel support (InputManager::
//     mouseWheelDelta()) is new this pass too.
//   - Starter loadout: pistol (primary), SMG (secondary), combat knife
//     (melee), all pre-equipped at startup — same out-of-the-box
//     "can fight from frame one" behavior earlier milestones had via
//     a hardcoded Weapon component, now flowing through the real
//     equip pipeline instead.

int main(int, char**) {
    engine::ApplicationConfig config;
    config.title = "wasteland2d — Milestone 6 (data layer)";
    config.width = 1280;
    config.height = 720;
    config.fixedUpdateHz = 60.0;

    engine::Application app(config);

    engine::ecs::Registry registry;
    engine::resource::TextureCache textures(app.window().renderer());
    engine::render::Camera camera(config.width, config.height);
    engine::world::TileMap map(textures, "assets/maps/sample_map.tmx");
    engine::physics::PhysicsWorld physics;

    game::data::ItemDatabase itemDb;
    itemDb.loadFromFile("assets/items/items.json");

    physics.createStaticBodiesFromRects(map.colliders());

    // --- player (root entity: movement, physics body, combat) ---
    double playerSpawnX = 4.0 * map.tileWidth();
    double playerSpawnY = 4.0 * map.tileHeight();

    engine::ecs::Entity player = registry.create();
    registry.emplace<engine::ecs::Transform>(player, playerSpawnX, playerSpawnY);
    registry.emplace<engine::render::Sprite>(player,
        engine::render::Sprite{24.0f, 24.0f, engine::render::Color{200, 60, 40, 255}, 1, 0.0f});
    // Owns this sprite's color entirely from here on — see the
    // FlashSystem::apply() call in the render callback below. base =
    // the sprite's normal color above; flash = the old muzzle-flash tint.
    registry.emplace<engine::fx::FlashEffect>(player,
        engine::fx::FlashEffect{engine::render::Color{200, 60, 40, 255},
                                 engine::render::Color{255, 240, 180, 255}, 0.06, 0.0});

    engine::physics::BodyParams playerParams;
    playerParams.type = engine::physics::BodyType::Dynamic;
    playerParams.position = glm::vec2(static_cast<float>(playerSpawnX), static_cast<float>(playerSpawnY));
    playerParams.width = 20.0f;
    playerParams.height = 20.0f;
    playerParams.density = 1.0f;
    playerParams.friction = 0.0f;
    // No linearDamping: velocity is set directly every fixed update
    // below (smoothed toward the input direction in code, not via
    // Box2D's damping integration), so a damping value here would never
    // get a window to act. Damping earns its keep once a body is
    // force-driven instead — see Milestone 7's vehicles.
    playerParams.fixedRotation = true;  // a top-down character doesn't tip over
    auto playerBody = physics.createBody(player, playerParams);
    registry.emplace<engine::physics::RigidBody>(player, playerBody);
    registry.emplace<game::components::Inventory>(player); // defaults: 6x4 grid, 40kg

    // --- starter loadout: primary (pistol), secondary (smg), melee
    //     (combat knife), all pre-equipped so the player can fight from
    //     frame one — same out-of-the-box behavior as earlier
    //     milestones' hardcoded Weapon, just now flowing through the
    //     real equip pipeline instead of a direct component emplace.
    //     Placed in the grid first, purely as the mechanism equip()
    //     expects (pull from a grid position) — they're immediately
    //     removed from the grid the instant they're equipped, so the
    //     player's starting inventory ends up with 0 stacks, not 3.
    registry.emplace<game::components::EquipmentSlots>(player);
    registry.emplace<game::components::QuickSlots>(player);
    auto& playerEquipment = registry.get<game::components::EquipmentSlots>(player);
    auto& playerQuickSlots = registry.get<game::components::QuickSlots>(player);
    // playerInventory here is deliberately scoped to setup only (this
    // block runs entirely before the game loop starts, so nothing can
    // invalidate it yet) — see the CRITICAL note below for why it must
    // never be captured this way again for anything used inside the
    // update/render loop.
    {
        auto& playerInventory = registry.get<game::components::Inventory>(player);
        game::systems::InventorySystem::addItem(playerInventory, itemDb, "pistol", 1);
        game::systems::InventorySystem::addItem(playerInventory, itemDb, "smg", 1);
        game::systems::InventorySystem::addItem(playerInventory, itemDb, "combat_knife", 1);
        game::systems::InventorySystem::addItem(playerInventory, itemDb, "school_backpack", 1);
        game::systems::InventorySystem::addItem(playerInventory, itemDb, "bandage", 3);

        auto equipStarterItem = [&](const std::string& itemId, game::components::EquipmentSlots::Slot slot) {
            for (const auto& stack : playerInventory.stacks) {
                if (stack.itemId == itemId) {
                    game::systems::EquipmentSystem::equip(playerInventory, playerEquipment, itemDb, slot,
                                                           stack.gridX, stack.gridY);
                    break; // equip() erases from playerInventory.stacks -- must stop iterating it immediately
                }
            }
        };
        equipStarterItem("pistol", game::components::EquipmentSlots::Slot::Primary);
        equipStarterItem("smg", game::components::EquipmentSlots::Slot::Secondary);
        equipStarterItem("combat_knife", game::components::EquipmentSlots::Slot::Melee);
        equipStarterItem("school_backpack", game::components::EquipmentSlots::Slot::Backpack);
    } // playerInventory (the reference above) ends its safe lifetime here — never used past this point

    playerEquipment.activeSlot = game::components::EquipmentSlots::Slot::Primary;
    game::systems::EquipmentSystem::syncActiveWeapon(registry, player, playerEquipment, itemDb);

    playerQuickSlots.itemIds[0] = "bandage"; // key 4 — bandages stay in the grid, just bound for quick-use

    // --- player "paperdoll" layers (Milestone 5.5 attachment demo) ---
    // Still colored-rectangle placeholders (Sprite.texture left null) —
    // real layered art is Milestone 6+ content. What's being proven
    // here is that Attachment/AttachmentSystem correctly keeps several
    // sprites moving together as one visual unit. Real stacking-order
    // tuning between layers (hair fully in front of/behind the body)
    // is an art-level concern for later, not attempted here.
    engine::ecs::Entity playerHair = registry.create();
    registry.emplace<engine::ecs::Transform>(playerHair, playerSpawnX, playerSpawnY);
    registry.emplace<engine::render::Sprite>(playerHair,
        engine::render::Sprite{12.0f, 12.0f, engine::render::Color{80, 50, 30, 255}, 1, 0.0f});
    registry.emplace<engine::ecs::Attachment>(playerHair,
        engine::ecs::Attachment{player, 0.0f, -12.0f, true});

    engine::ecs::Entity playerPants = registry.create();
    registry.emplace<engine::ecs::Transform>(playerPants, playerSpawnX, playerSpawnY);
    registry.emplace<engine::render::Sprite>(playerPants,
        engine::render::Sprite{18.0f, 12.0f, engine::render::Color{50, 55, 70, 255}, 1, 0.0f});
    registry.emplace<engine::ecs::Attachment>(playerPants,
        engine::ecs::Attachment{player, 0.0f, 10.0f, true});

    // Weapon: followRotation = false — AttachmentSystem keeps its
    // position pinned to the player, but never touches its rotation.
    // The update loop below sets rotationDegrees from aimDir every
    // frame instead, so it tracks the mouse independent of movement.
    engine::ecs::Entity playerWeapon = registry.create();
    registry.emplace<engine::ecs::Transform>(playerWeapon, playerSpawnX, playerSpawnY);
    registry.emplace<engine::render::Sprite>(playerWeapon,
        engine::render::Sprite{16.0f, 5.0f, engine::render::Color{40, 40, 45, 255}, 1, 0.0f});
    registry.emplace<engine::ecs::Attachment>(playerWeapon,
        engine::ecs::Attachment{player, 0.0f, 0.0f, false});

    // --- tall decorative "tree" — Y-sort demo, no physics body ---
    engine::ecs::Entity tree = registry.create();
    registry.emplace<engine::ecs::Transform>(tree, 10.0 * map.tileWidth(), 8.0 * map.tileHeight());
    engine::render::Sprite treeSprite{28.0f, 48.0f, engine::render::Color{74, 54, 38, 255}, 1, 0.0f};
    treeSprite.sortOriginYOffset = 18.0f; // push the sort point toward the trunk's base, not the canopy's center
    registry.emplace<engine::render::Sprite>(tree, treeSprite);

    // --- static "vehicle placeholder" — trigger/sensor + Interactable demo ---
    engine::ecs::Entity vehiclePlaceholder = registry.create();
    registry.emplace<engine::ecs::Transform>(vehiclePlaceholder, 14.0 * map.tileWidth(), 10.0 * map.tileHeight());
    registry.emplace<engine::render::Sprite>(vehiclePlaceholder,
        engine::render::Sprite{48.0f, 28.0f, engine::render::Color{90, 95, 100, 255}, 1, 0.0f});
    registry.emplace<engine::physics::Interactable>(vehiclePlaceholder,
        engine::physics::Interactable{"Press E to enter (Milestone 7)"});

    engine::physics::BodyParams vehicleParams;
    vehicleParams.type = engine::physics::BodyType::Static; // becomes Dynamic + Drivable in Milestone 7
    vehicleParams.position = glm::vec2(14.0f * map.tileWidth(), 10.0f * map.tileHeight());
    vehicleParams.width = 48.0f;
    vehicleParams.height = 28.0f;
    auto vehicleBody = physics.createBody(vehiclePlaceholder, vehicleParams);
    physics.addCircleSensor(vehicleBody, 60.0f); // interaction range, larger than the solid body

    // --- target dummy — static, Health, purely something to shoot ---
    // Real AI/patrol is Milestone 8; this exists solely to confirm the
    // hitscan/damage/death pipeline works end to end.
    engine::ecs::Entity dummy = registry.create();
    registry.emplace<engine::ecs::Transform>(dummy, 8.0 * map.tileWidth(), 4.0 * map.tileHeight());
    registry.emplace<engine::render::Sprite>(dummy,
        engine::render::Sprite{22.0f, 22.0f, engine::render::Color{200, 60, 60, 255}, 1, 0.0f});
    registry.emplace<game::components::Health>(dummy, game::components::Health{60.0f, 60.0f, false});
    // What this specific dummy drops on death — CombatSystem itself
    // stays generic (see LootDrop.h); this is the content decision.
    registry.emplace<game::components::LootDrop>(dummy,
        game::components::LootDrop{{{"bandage", 2}, {"9mm_ammo", 30}, {"pistol", 1}}});

    engine::physics::BodyParams dummyParams;
    dummyParams.type = engine::physics::BodyType::Static;
    dummyParams.position = glm::vec2(8.0f * map.tileWidth(), 4.0f * map.tileHeight());
    dummyParams.width = 22.0f;
    dummyParams.height = 22.0f;
    auto dummyBody = physics.createBody(dummy, dummyParams);
    registry.emplace<engine::physics::RigidBody>(dummy, dummyBody);

    engine::physics::InteractionTracker interactionTracker;
    bool nearVehicle = false;

    const double playerSpeed = 240.0;
    // Exponential accel/decel rates (higher = snappier). Separate knobs
    // so starting can feel a touch quicker than stopping, or vice versa
    // — tune both by eye; this is the actual "movement feel" control.
    const float playerAccel = 22.0f;
    const float playerDecel = 16.0f;

    // Milestone 5.5: combat feedback now runs through the generic
    // engine::fx primitives instead of Milestone 5's hand-rolled
    // timers — same visible behavior, shared machinery any future
    // effect (an NPC's weapon, a vehicle impact) can reuse instead of
    // copy-pasting this again.
    engine::fx::ScreenShake screenShake;
    engine::fx::WorldLineEffects tracerEffects;

    // Camera lean/zoom lerp speeds while ADS is held/released — gentler
    // than the movement-follow rate so quick aim flicks don't visibly
    // jitter the camera. How *far* it leans and *how much* it zooms are
    // per-weapon (Weapon::adsOffsetPixels/adsZoomMultiplier) — see the
    // ADS handling below.
    const double cameraAimLerpSpeed = 5.0;
    const double cameraZoomLerpSpeed = 6.0;

    // Custom cursor demo: a small tinted square via the shared
    // whitePixel texture — same placeholder pattern as every other
    // entity in this file. Swapped for the OS cursor while the
    // inventory is open (see below) — the "policy" half of the
    // capability Milestone 5.5 built but deliberately didn't wire up
    // yet, now that there's a real trigger condition for it.
    engine::input::Cursor cursor;
    cursor.set(textures.whitePixel(), engine::render::Color{255, 255, 255, 220}, 6, 6);

    // --- inventory UI state ---
    bool inventoryOpen = false;
    game::ui::DragDropController dragDrop;

    // UI text — VT323 (SIL OFL licensed, bundled under assets/fonts/)
    // is a terminal/typewriter-style font, a reasonable fit for a
    // wasteland survival game and legible even small. One Font at one
    // size for now; a real UI would want a couple of sizes (a small
    // one for quantities, a larger one for headers) — not attempted
    // here, this is the first use of text rendering in the project.
    engine::render::Font uiFont("assets/fonts/VT323-Regular.ttf", 20);
    engine::render::TextRenderer textRenderer(app.window().renderer());

    // Fixed screen positions — matches this file's general "colored
    // rectangle placeholder" approach; a real UI layout system is
    // future work, not attempted here. lootPanelLayout's dimensions
    // (4x3) match what CombatSystem::spawnCorpse always hardcodes for
    // a corpse's Inventory; if corpse size ever varies, this would need
    // to be re-read from whichever corpse is actually nearby instead of
    // fixed here.
    engine::ui::GridLayout playerPanelLayout;
    playerPanelLayout.screenX = 60;
    playerPanelLayout.screenY = 120;
    playerPanelLayout.cellSize = 44;
    playerPanelLayout.gridWidth = registry.get<game::components::Inventory>(player).gridWidth;
    playerPanelLayout.gridHeight = registry.get<game::components::Inventory>(player).gridHeight;

    engine::ui::GridLayout lootPanelLayout;
    lootPanelLayout.screenX = playerPanelLayout.screenX + playerPanelLayout.cellSize * playerPanelLayout.gridWidth + 40;
    lootPanelLayout.screenY = playerPanelLayout.screenY;
    lootPanelLayout.cellSize = 44;
    lootPanelLayout.gridWidth = 4;
    lootPanelLayout.gridHeight = 3;

    // Equipment slots (Primary, Secondary, Melee, Backpack) — a 4x1
    // row directly above the player's grid panel; only visible while
    // the inventory is open, same as the grid panels themselves.
    engine::ui::GridLayout equipmentPanelLayout;
    equipmentPanelLayout.screenX = playerPanelLayout.screenX;
    equipmentPanelLayout.screenY = playerPanelLayout.screenY - 60;
    equipmentPanelLayout.cellSize = 48;
    equipmentPanelLayout.gridWidth = 4;
    equipmentPanelLayout.gridHeight = 1;

    // Hotbar (keys 4-9) — bottom of the screen, drawn every frame
    // regardless of inventoryOpen (see the render callback below), the
    // same way any action game's quick-slot bar stays visible during
    // normal play.
    engine::ui::GridLayout hotbarLayout;
    hotbarLayout.cellSize = 48;
    hotbarLayout.gridWidth = game::components::QuickSlots::kSlotCount;
    hotbarLayout.gridHeight = 1;
    hotbarLayout.screenX = (config.width - hotbarLayout.cellSize * hotbarLayout.gridWidth) / 2;
    hotbarLayout.screenY = config.height - hotbarLayout.cellSize - 20;

    // Which Lootable entity (if any) the loot panel currently shows —
    // updated once per fixed tick from InteractionTracker, read by both
    // the update callback (drag/drop targeting) and the render callback
    // (whether to draw the second panel at all). Same pattern as
    // nearVehicle below.
    engine::ecs::Entity nearbyLootEntity = engine::ecs::kNullEntity;

    // Given a screen point, finds which visible panel (if any) it falls
    // within and returns that panel's backing Inventory + grid cell.
    // Player panel takes priority when panels would ever overlap (they
    // don't, at these fixed positions, but priority order matters if
    // that changes). Returns std::nullopt if the point isn't over any
    // currently-visible panel.
    // Which panel a screen point landed in — needed for quick-grab/
    // quick-store (Ctrl+click) to know which inventory is "the other
    // one" to send an item to.
    enum class PanelSide { Player, Loot };

    auto pickPanelCell =
        [&](int screenPointX,
            int screenPointY) -> std::optional<std::tuple<PanelSide, game::components::Inventory*, std::pair<int, int>>> {
        if (auto cell = playerPanelLayout.cellAt(screenPointX, screenPointY)) {
            return std::make_tuple(PanelSide::Player, &registry.get<game::components::Inventory>(player), *cell);
        }
        if (nearbyLootEntity != engine::ecs::kNullEntity && registry.has<game::components::Inventory>(nearbyLootEntity)) {
            if (auto cell = lootPanelLayout.cellAt(screenPointX, screenPointY)) {
                return std::make_tuple(PanelSide::Loot, &registry.get<game::components::Inventory>(nearbyLootEntity),
                                        *cell);
            }
        }
        return std::nullopt;
    };

    // Shared with the render callback below, so the custom cursor draws
    // at the same position aim/fire used this tick rather than lagging
    // a frame behind.
    int mouseX = 0;
    int mouseY = 0;

    app.setUpdateCallback([&](double dt) {
        engine::InputManager& input = app.input();

        // Inventory toggle — placed first so inventoryOpen's new state
        // this frame consistently gates everything below (movement,
        // fire, ADS), not just next frame.
        if (input.wasPressed(engine::Action::Inventory)) {
            inventoryOpen = !inventoryOpen;
            if (inventoryOpen) {
                cursor.showSystemCursor(); // precise UI interaction needs the real cursor, not the aim crosshair
            } else {
                cursor.set(textures.whitePixel(), engine::render::Color{255, 255, 255, 220}, 6, 6);
                dragDrop.cancelDrag(); // closing mid-drag returns whatever was held to where it came from
            }
        }

        glm::vec2 dir(0.0f, 0.0f);
        if (input.isHeld(engine::Action::MoveUp)) dir.y -= 1.0f;
        if (input.isHeld(engine::Action::MoveDown)) dir.y += 1.0f;
        if (input.isHeld(engine::Action::MoveLeft)) dir.x -= 1.0f;
        if (input.isHeld(engine::Action::MoveRight)) dir.x += 1.0f;

        if (dir.x != 0.0f || dir.y != 0.0f) {
            dir = glm::normalize(dir);
        }
        if (inventoryOpen) dir = glm::vec2(0.0f, 0.0f); // menu-open pause: no movement while managing inventory

        float speed = static_cast<float>(playerSpeed);
        if (input.isHeld(engine::Action::Sprint)) speed *= 1.6f;
        // Carry-weight penalty — multiplicative with sprint rather than
        // an explicit "can't sprint while overloaded" block: at the
        // encumbrance floor (~5% speed), a 1.6x sprint multiplier on
        // top of that is still negligible in practice, so the one rule
        // covers both cases without special-casing sprint separately.
        // Re-fetched fresh here rather than a cached reference — see
        // Registry::get()'s warning: a corpse spawning earlier this
        // same tick (Damage::apply, via a kill) emplaces its own
        // Inventory component, which can invalidate any long-lived
        // reference to the player's.
        speed *= game::systems::Encumbrance::speedMultiplier(registry.get<game::components::Inventory>(player), itemDb);

        // Exponentially smooth velocity toward the input direction
        // instead of snapping to it instantly — same technique
        // Camera::followSmooth already uses, applied to velocity rather
        // than position. Moving uses playerAccel, releasing uses
        // playerDecel, so starting and stopping can be tuned separately.
        glm::vec2 targetVelocity = dir * speed;
        glm::vec2 currentVelocity = physics.linearVelocity(playerBody);
        bool isMoving = (dir.x != 0.0f || dir.y != 0.0f);
        float smoothRate = isMoving ? playerAccel : playerDecel;
        float smoothT = 1.0f - std::exp(-smoothRate * static_cast<float>(dt));
        glm::vec2 newVelocity = currentVelocity + (targetVelocity - currentVelocity) * smoothT;

        physics.setLinearVelocity(playerBody, newVelocity);
        physics.step(dt);
        engine::physics::PhysicsSyncSystem::sync(registry, physics);

        auto& transform = registry.get<engine::ecs::Transform>(player);

        // --- aim (independent of movement direction) ---
        input.getMousePosition(mouseX, mouseY);
        double aimWorldX = 0.0, aimWorldY = 0.0;
        camera.screenToWorld(static_cast<double>(mouseX), static_cast<double>(mouseY), aimWorldX, aimWorldY);

        glm::vec2 playerPos(static_cast<float>(transform.x), static_cast<float>(transform.y));
        glm::vec2 aimDir(static_cast<float>(aimWorldX) - playerPos.x, static_cast<float>(aimWorldY) - playerPos.y);
        if (glm::length(aimDir) > 0.0001f) aimDir = glm::normalize(aimDir);

        // Camera follow + aim-offset lean. followSmoothWithOffset biases
        // the usual position-follow toward playerPos + aimDir * offset,
        // so the camera subtly favors showing more of what's ahead of
        // where you're aiming.
        // Aim-down-sights: right-click held zooms in and leans the
        // camera toward the aim direction, both scaled by the equipped
        // weapon's Weapon::adsZoomMultiplier/adsOffsetPixels — a pistol
        // barely moves, an AR/sniper would (once Milestone 6's
        // equipment slots let more than one Weapon exist at a time)
        // lean and zoom further, by simply carrying different data on
        // this same field. Releasing right-click smoothly relaxes back
        // to zoom 1.0 / no lean, using the same lerp helpers.
        bool isAiming = !inventoryOpen && input.isMouseButtonHeld(SDL_BUTTON_RIGHT);
        float adsZoomMultiplier = 1.0f;
        float adsOffsetPixels = 0.0f;
        if (isAiming && registry.has<game::components::Weapon>(player)) {
            const auto& weapon = registry.get<game::components::Weapon>(player);
            adsZoomMultiplier = weapon.adsZoomMultiplier;
            adsOffsetPixels = weapon.adsOffsetPixels;
        }

        glm::vec2 cameraOffset = isAiming ? aimDir * adsOffsetPixels : glm::vec2(0.0f, 0.0f);
        camera.followSmoothWithOffset(transform.x, transform.y, cameraOffset.x, cameraOffset.y, dt,
                                       cameraAimLerpSpeed);
        camera.zoomSmooth(isAiming ? adsZoomMultiplier : 1.0, dt, cameraZoomLerpSpeed);

        // AttachmentSystem after physics sync (parent Transform is
        // current) and after the camera/aim math above, so the weapon's
        // position for this tick is settled before its rotation is set
        // below.
        engine::ecs::AttachmentSystem::update(registry);

        // Weapon tracks the mouse independent of body/movement facing —
        // atan2(y, x) in degrees matches SDL_RenderCopyExF's clockwise
        // convention directly in this Y-down world (see the Milestone 4
        // audit note on Box2D's rotation convention lining up with SDL's
        // for the same reason).
        if (registry.has<engine::ecs::Transform>(playerWeapon)) {
            auto& weaponTransform = registry.get<engine::ecs::Transform>(playerWeapon);
            weaponTransform.rotationDegrees =
                std::atan2(aimDir.y, aimDir.x) * (180.0 / 3.14159265358979323846);
        }

        // --- fire / swing ---
        game::systems::CombatSystem::updateCooldowns(registry, dt);
        game::systems::MeleeCombatSystem::updateCooldowns(registry, dt);
        engine::fx::FlashSystem::tick(registry, dt);

        bool wantsToAttack =
            !inventoryOpen && (input.isMouseButtonHeld(SDL_BUTTON_LEFT) || input.isHeld(engine::Action::Fire));
        if (wantsToAttack) {
            if (playerEquipment.activeSlot == game::components::EquipmentSlots::Slot::Melee) {
                auto swingResult =
                    game::systems::MeleeCombatSystem::swing(registry, physics, itemDb, player, playerPos, aimDir);
                if (swingResult.fired) {
                    registry.get<engine::fx::FlashEffect>(player).trigger();
                    // No tracer — a swing is a short-range arc, not a
                    // ray; a real swing-arc visual is future work (see
                    // MeleeWeapon.h's swingDuration, currently unused
                    // by anything visual yet).
                }
                if (!swingResult.hitEntities.empty()) {
                    screenShake.trigger(swingResult.killedAny ? 7.0 : 3.0, swingResult.killedAny ? 0.14 : 0.07);
                    std::printf("[combat] melee hit %zu target(s) for %.0f damage%s\n",
                                swingResult.hitEntities.size(), static_cast<double>(swingResult.totalDamageDealt),
                                swingResult.killedAny ? " -- target down" : "");
                }
            } else {
                auto fireResult =
                    game::systems::CombatSystem::fireWeapon(registry, physics, itemDb, player, playerPos, aimDir);
                if (fireResult.fired) {
                    registry.get<engine::fx::FlashEffect>(player).trigger();
                    // Tracer: the hitscan ray itself is otherwise completely
                    // invisible (nothing travels — that's expected for a
                    // hitscan weapon), so draw the path it took as a
                    // short-lived line. shotEnd is where CombatSystem
                    // already worked out the ray actually stopped, hit or not.
                    tracerEffects.trigger(playerPos, fireResult.shotEnd,
                                           engine::render::Color{255, 235, 160, 255}, 0.05);
                }
                if (fireResult.hit) {
                    screenShake.trigger(fireResult.killedTarget ? 7.0 : 3.0, fireResult.killedTarget ? 0.14 : 0.07);
                    if (fireResult.damageDealt > 0.0f) {
                        std::printf("[combat] hit for %.0f damage%s\n", fireResult.damageDealt,
                                    fireResult.killedTarget ? " -- target down" : "");
                    } else {
                        std::printf("[combat] shot hit environment\n");
                    }
                }
            }
        }

        tracerEffects.tick(dt);
        glm::vec2 shakeOffset = screenShake.tick(dt);
        camera.setShakeOffset(shakeOffset.x, shakeOffset.y);

        auto events = physics.drainTriggerEvents();
        for (const auto& event : events) {
            engine::ecs::Entity other = engine::ecs::kNullEntity;
            if (event.a == player) other = event.b;
            else if (event.b == player) other = event.a;
            if (other != engine::ecs::kNullEntity && registry.has<engine::physics::Interactable>(other)) {
                const auto& interactable = registry.get<engine::physics::Interactable>(other);
                if (event.began) {
                    std::printf("[interact] %s\n", interactable.promptText.c_str());
                } else {
                    std::printf("[interact] (out of range)\n");
                }
            }
        }
        interactionTracker.update(player, events);
        nearVehicle = interactionTracker.nearby().count(vehiclePlaceholder) > 0;

        nearbyLootEntity = engine::ecs::kNullEntity;
        for (engine::ecs::Entity other : interactionTracker.nearby()) {
            if (registry.has<game::components::Lootable>(other) && registry.has<game::components::Inventory>(other)) {
                nearbyLootEntity = other;
                break; // first one found — multiple simultaneous lootables in range isn't a case this demo scene creates
            }
        }

        // Looting: press E while near a Lootable + Inventory entity (a
        // corpse, from CombatSystem's death handling above) to pull
        // everything it's carrying into the player's own Inventory,
        // respecting grid space and weight — see
        // InventorySystem::moveAllTo. This console-printed transfer is
        // a stand-in for a real drag-and-drop inventory UI, which
        // doesn't exist yet (a full engine::ui widget layer is a much
        // larger, separate piece of Milestone 6 not attempted in this
        // pass) — the data layer underneath it (ItemDatabase,
        // Inventory, InventorySystem) is exactly what that UI would
        // call into once it exists, and is already fully wired here.
        if (input.wasPressed(engine::Action::Interact)) {
            for (engine::ecs::Entity other : interactionTracker.nearby()) {
                if (!registry.has<game::components::Lootable>(other) ||
                    !registry.has<game::components::Inventory>(other)) {
                    continue;
                }
                auto& lootInventory = registry.get<game::components::Inventory>(other);
                if (lootInventory.stacks.empty()) {
                    std::printf("[loot] nothing left to take\n");
                    continue;
                }

                int stacksBefore = static_cast<int>(lootInventory.stacks.size());
                auto& playerInventory = registry.get<game::components::Inventory>(player);
                game::systems::InventorySystem::moveAllTo(lootInventory, playerInventory, itemDb);
                int stacksLeft = static_cast<int>(lootInventory.stacks.size());

                if (stacksLeft == 0) {
                    std::printf("[loot] took everything (%d stack%s)\n", stacksBefore,
                                stacksBefore == 1 ? "" : "s");
                } else {
                    std::printf("[loot] took what fit -- %d stack%s left behind (inventory full or too heavy)\n",
                                stacksLeft, stacksLeft == 1 ? "" : "s");
                }
            }
        }

        // Drag-and-drop / quick-transfer / rotate: all only interpreted
        // as inventory interaction while the panel is open (fire is
        // already gated off above, so there's no ambiguity with
        // shooting).
        if (inventoryOpen) {
            // Rotate the held item — only meaningful mid-drag; no-ops
            // otherwise via DragDropController::toggleRotation itself.
            if (input.wasPressed(engine::Action::RotateItem)) {
                dragDrop.toggleRotation();
            }

            if (input.wasMouseButtonPressed(SDL_BUTTON_LEFT)) {
                if (auto target = pickPanelCell(mouseX, mouseY)) {
                    auto [side, inv, cell] = *target;

                    if (input.isCtrlHeld() && !dragDrop.isDragging()) {
                        // Quick-grab/quick-store: instantly send
                        // whatever's under the cursor to "the other"
                        // panel instead of starting a drag. No-op if
                        // there's no other panel visible (e.g.
                        // Ctrl+clicking your own inventory with no
                        // lootable nearby) or the cell is empty.
                        game::components::Inventory* other = nullptr;
                        if (side == PanelSide::Player && nearbyLootEntity != engine::ecs::kNullEntity &&
                            registry.has<game::components::Inventory>(nearbyLootEntity)) {
                            other = &registry.get<game::components::Inventory>(nearbyLootEntity);
                        } else if (side == PanelSide::Loot) {
                            other = &registry.get<game::components::Inventory>(player);
                        }
                        if (other) {
                            game::systems::InventorySystem::quickTransferStack(*inv, *other, itemDb, cell.first,
                                                                                cell.second);
                        }
                    } else {
                        dragDrop.beginDrag(itemDb, *inv, cell.first, cell.second);
                    }
                }
            }
            if (input.wasMouseButtonReleased(SDL_BUTTON_LEFT)) {
                if (dragDrop.isDragging()) {
                    if (auto target = pickPanelCell(mouseX, mouseY)) {
                        auto [side, inv, cell] = *target;
                        (void)side;
                        auto [resolvedX, resolvedY] = dragDrop.resolveDropTopLeft(cell.first, cell.second);
                        dragDrop.endDrag(itemDb, *inv, resolvedX, resolvedY);
                    } else {
                        dragDrop.cancelDrag(); // released outside every visible panel — return to source
                    }
                }
            }

            // Equip/unequip: press 1/2/3 while hovering a compatible
            // weapon (and not mid-drag) to equip it; press the same
            // key while NOT hovering anything unequips that slot
            // instead (back to the grid) — one key, two directions,
            // decided by whether there's something valid under the
            // cursor. See EquipmentSystem::equip for what "compatible"
            // means (right WeaponStats::kind for the slot,
            // non-stackable). Deliberately does NOT also switch
            // activeSlot — equipping and switching are different
            // actions (see main.cpp's header comment) — so the
            // newly-equipped weapon only takes effect immediately if
            // it happened to land in the slot that was already active.
            if (!dragDrop.isDragging()) {
                std::optional<game::components::EquipmentSlots::Slot> pressedSlot;
                if (input.wasPressed(engine::Action::Slot1)) pressedSlot = game::components::EquipmentSlots::Slot::Primary;
                else if (input.wasPressed(engine::Action::Slot2)) pressedSlot = game::components::EquipmentSlots::Slot::Secondary;
                else if (input.wasPressed(engine::Action::Slot3)) pressedSlot = game::components::EquipmentSlots::Slot::Melee;

                if (pressedSlot.has_value()) {
                    auto target = pickPanelCell(mouseX, mouseY);
                    bool hoveringAnItem =
                        target.has_value() &&
                        game::systems::InventorySystem::stackIndexAt(*std::get<1>(*target), itemDb,
                                                                       std::get<2>(*target).first,
                                                                       std::get<2>(*target).second)
                            .has_value();

                    if (hoveringAnItem) {
                        auto [side, inv, cell] = *target;
                        (void)side;
                        bool equipped = game::systems::EquipmentSystem::equip(*inv, playerEquipment, itemDb,
                                                                               *pressedSlot, cell.first, cell.second);
                        if (equipped) {
                            std::printf("[equip] equipped\n");
                            if (playerEquipment.activeSlot == *pressedSlot) {
                                game::systems::EquipmentSystem::syncActiveWeapon(registry, player, playerEquipment,
                                                                                  itemDb);
                            }
                        } else {
                            std::printf("[equip] can't equip that there -- wrong type for this slot\n");
                        }
                    } else {
                        bool unequipped = game::systems::EquipmentSystem::unequip(
                            registry.get<game::components::Inventory>(player), playerEquipment, itemDb, *pressedSlot);
                        if (unequipped) {
                            std::printf("[equip] unequipped\n");
                            if (playerEquipment.activeSlot == *pressedSlot) {
                                game::systems::EquipmentSystem::syncActiveWeapon(registry, player, playerEquipment,
                                                                                  itemDb);
                            }
                        }
                        // No feedback on a no-op unequip (slot was
                        // already empty) — nothing happened, nothing
                        // to report, same as every other no-op gesture
                        // in this file.
                    }
                }

                // Backpack has no numbered hotkey (1-3 are weapons,
                // 4-9 are quick slots) — press B instead, same
                // hover-to-equip/empty-to-unequip gesture.
                if (input.wasPressed(engine::Action::EquipBackpack)) {
                    auto target = pickPanelCell(mouseX, mouseY);
                    bool hoveringAnItem =
                        target.has_value() &&
                        game::systems::InventorySystem::stackIndexAt(*std::get<1>(*target), itemDb,
                                                                       std::get<2>(*target).first,
                                                                       std::get<2>(*target).second)
                            .has_value();

                    if (hoveringAnItem) {
                        auto [side, inv, cell] = *target;
                        (void)side;
                        bool equipped = game::systems::EquipmentSystem::equip(
                            *inv, playerEquipment, itemDb, game::components::EquipmentSlots::Slot::Backpack,
                            cell.first, cell.second);
                        // No syncActiveWeapon call needed either way —
                        // Backpack is never the active slot, so there's
                        // no live Weapon/MeleeWeapon component to
                        // refresh.
                        std::printf(equipped ? "[equip] backpack equipped\n"
                                              : "[equip] can't equip that as a backpack\n");
                    } else {
                        bool unequipped = game::systems::EquipmentSystem::unequip(
                            registry.get<game::components::Inventory>(player), playerEquipment, itemDb,
                            game::components::EquipmentSlots::Slot::Backpack);
                        if (unequipped) std::printf("[equip] backpack unequipped\n");
                    }
                }

                // Quick-slot assign/clear: Ctrl+4..9 while hovering a
                // usable item (has ItemDefinition::useEffect — no
                // point binding something UseItemSystem could never do
                // anything with) binds it to that hotbar slot; Ctrl+
                // the same key while not hovering one clears it.
                // Doesn't touch the inventory grid either way — see
                // QuickSlots.h.
                if (input.isCtrlHeld()) {
                    int quickSlotIndex = -1;
                    if (input.wasPressed(engine::Action::Slot4)) quickSlotIndex = 0;
                    else if (input.wasPressed(engine::Action::Slot5)) quickSlotIndex = 1;
                    else if (input.wasPressed(engine::Action::Slot6)) quickSlotIndex = 2;
                    else if (input.wasPressed(engine::Action::Slot7)) quickSlotIndex = 3;
                    else if (input.wasPressed(engine::Action::Slot8)) quickSlotIndex = 4;
                    else if (input.wasPressed(engine::Action::Slot9)) quickSlotIndex = 5;

                    if (quickSlotIndex >= 0) {
                        bool assigned = false;
                        if (auto target = pickPanelCell(mouseX, mouseY)) {
                            auto [side, inv, cell] = *target;
                            (void)side;
                            if (auto idx = game::systems::InventorySystem::stackIndexAt(*inv, itemDb, cell.first,
                                                                                         cell.second)) {
                                const auto& stack = inv->stacks[*idx];
                                const game::data::ItemDefinition* def = itemDb.find(stack.itemId);
                                if (def && def->useEffect.has_value()) {
                                    playerQuickSlots.itemIds[static_cast<std::size_t>(quickSlotIndex)] = stack.itemId;
                                    assigned = true;
                                    std::printf("[quickslot] bound key %d\n", quickSlotIndex + 4);
                                }
                            }
                        }
                        if (!assigned) {
                            auto& slot = playerQuickSlots.itemIds[static_cast<std::size_t>(quickSlotIndex)];
                            if (!slot.empty()) {
                                slot.clear();
                                std::printf("[quickslot] cleared key %d\n", quickSlotIndex + 4);
                            }
                        }
                    }
                }
            }
        } else {
            // Switching active weapon (inventory closed, normal
            // gameplay) — a completely different action from equipping
            // above: this never touches the inventory grid, it only
            // changes which already-equipped slot is in-hand. Both
            // number keys and the scroll wheel work, per your call.
            game::components::EquipmentSlots::Slot newActive = playerEquipment.activeSlot;
            if (input.wasPressed(engine::Action::Slot1)) {
                newActive = game::components::EquipmentSlots::Slot::Primary;
            } else if (input.wasPressed(engine::Action::Slot2)) {
                newActive = game::components::EquipmentSlots::Slot::Secondary;
            } else if (input.wasPressed(engine::Action::Slot3)) {
                newActive = game::components::EquipmentSlots::Slot::Melee;
            } else if (int wheel = input.mouseWheelDelta(); wheel != 0) {
                int current = static_cast<int>(playerEquipment.activeSlot);
                int next = (current + (wheel > 0 ? 1 : -1) + 3) % 3;
                newActive = static_cast<game::components::EquipmentSlots::Slot>(next);
            }
            if (newActive != playerEquipment.activeSlot) {
                playerEquipment.activeSlot = newActive;
                game::systems::EquipmentSystem::syncActiveWeapon(registry, player, playerEquipment, itemDb);
                const char* slotNames[] = {"primary", "secondary", "melee"};
                std::printf("[equip] switched to %s\n", slotNames[static_cast<int>(newActive)]);
            }

            // Quick-slot use — 4..9 with the inventory closed. Console
            // output stands in for a real notification/toast UI (which
            // doesn't exist yet), same pattern as the [combat]/[loot]
            // lines elsewhere in this file.
            int usedSlotIndex = -1;
            if (input.wasPressed(engine::Action::Slot4)) usedSlotIndex = 0;
            else if (input.wasPressed(engine::Action::Slot5)) usedSlotIndex = 1;
            else if (input.wasPressed(engine::Action::Slot6)) usedSlotIndex = 2;
            else if (input.wasPressed(engine::Action::Slot7)) usedSlotIndex = 3;
            else if (input.wasPressed(engine::Action::Slot8)) usedSlotIndex = 4;
            else if (input.wasPressed(engine::Action::Slot9)) usedSlotIndex = 5;

            if (usedSlotIndex >= 0) {
                auto useResult = game::systems::UseItemSystem::useQuickSlot(
                    registry, player, registry.get<game::components::Inventory>(player), playerQuickSlots, itemDb,
                    usedSlotIndex);
                if (useResult.used) {
                    if (useResult.healedAmount > 0.0f) {
                        std::printf("[use] healed %.0f\n", static_cast<double>(useResult.healedAmount));
                    } else {
                        std::printf("[use] used item in slot %d\n", usedSlotIndex + 4);
                    }
                }
            }
        }

        if (input.wasPressed(engine::Action::Pause)) {
            app.quit();
        }
    });

    app.setRenderCallback([&](engine::Window& window, double /*alpha*/) {
        window.clear(24, 26, 22, 255); // dark olive — placeholder "wasteland" backdrop
        map.render(window.renderer(), camera);

        // Brighten the vehicle placeholder while the player is in its
        // sensor range — a cheap visual proof that InteractionTracker's
        // state is live and correct, without needing text rendering
        // (that's Milestone 6's UI system).
        auto& vehicleSprite = registry.get<engine::render::Sprite>(vehiclePlaceholder);
        vehicleSprite.color = nearVehicle ? engine::render::Color{160, 170, 180, 255}
                                           : engine::render::Color{90, 95, 100, 255};

        // Milestone 5.5: the player's muzzle flash is now owned by its
        // FlashEffect component — this one call replaces the manual
        // "if (muzzleFlashTimer > 0.0) sprite.color = ..." block
        // Milestone 5 wrote by hand, and will apply to any future
        // FlashEffect-bearing entity (an NPC, say) automatically.
        engine::fx::FlashSystem::apply(registry);

        // Darken any Health-bearing sprite toward its base tone as it
        // takes damage — visible proof a hit landed without needing
        // Milestone 6's UI system for a real health bar yet. Left as
        // game-level code rather than folded into FlashEffect: this is
        // a continuous function of a Health value, not a timed
        // two-state flash — a genuinely different shape of effect (see
        // FlashEffect.h's own note on this).
        for (engine::ecs::Entity e : registry.view<game::components::Health, engine::render::Sprite>()) {
            const auto& health = registry.get<game::components::Health>(e);
            auto& sprite = registry.get<engine::render::Sprite>(e);
            float healthFraction = health.max > 0.0f ? health.current / health.max : 0.0f;
            auto shade = static_cast<std::uint8_t>(60.0f + 140.0f * healthFraction);
            sprite.color = engine::render::Color{shade, static_cast<std::uint8_t>(shade * 0.35f),
                                                  static_cast<std::uint8_t>(shade * 0.35f), 255};
        }

        engine::render::SpriteRenderSystem::render(registry, window, textures, camera);

        // Tracer: hitscan resolves instantly, so without this the shot
        // is completely invisible — nothing travels from muzzle to
        // target. WorldLineEffects owns the fade/draw now instead of
        // main.cpp hand-rolling an SDL_RenderDrawLineF call — not a
        // real projectile, Milestone 5 stays hitscan-only per
        // ROADMAP.md; travel-time projectile weapons are a later,
        // separate Weapon type.
        tracerEffects.render(window.renderer(), camera);

        // Inventory UI — drawn in screen space (no camera transform),
        // on top of the game world but underneath the cursor. Text
        // (quantities, the weight readout, hover tooltips) now renders
        // for real via engine::render::Font/TextRenderer — see the
        // header comment.
        if (inventoryOpen) {
            SDL_Renderer* renderer = window.renderer();

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 10, 10, 12, 160);
            SDL_FRect backdrop{0.0f, 0.0f, static_cast<float>(config.width), static_cast<float>(config.height)};
            SDL_RenderFillRectF(renderer, &backdrop);

            engine::ui::renderGridCells(renderer, playerPanelLayout, engine::render::Color{50, 48, 45, 230},
                                         engine::render::Color{100, 96, 90, 255});
            game::ui::renderInventoryContents(renderer, playerPanelLayout, registry.get<game::components::Inventory>(player),
                                               itemDb);
            game::ui::renderStackQuantities(playerPanelLayout, registry.get<game::components::Inventory>(player),
                                             itemDb, uiFont, textRenderer);
            // Only the player's own panel gets a weight readout — a
            // corpse's Inventory (see CombatSystem::spawnCorpse) uses
            // an effectively-unlimited maxWeight, since a body isn't
            // "carried"; a number against that limit wouldn't mean
            // anything useful to show.
            game::ui::renderWeightReadout(playerPanelLayout, registry.get<game::components::Inventory>(player),
                                           itemDb, uiFont, textRenderer);
            game::ui::renderEquipmentSlots(renderer, equipmentPanelLayout, playerEquipment, itemDb);

            if (nearbyLootEntity != engine::ecs::kNullEntity && registry.has<game::components::Inventory>(nearbyLootEntity)) {
                engine::ui::renderGridCells(renderer, lootPanelLayout, engine::render::Color{50, 48, 45, 230},
                                             engine::render::Color{100, 96, 90, 255});
                game::ui::renderInventoryContents(renderer, lootPanelLayout,
                                                   registry.get<game::components::Inventory>(nearbyLootEntity), itemDb);
                game::ui::renderStackQuantities(lootPanelLayout, registry.get<game::components::Inventory>(nearbyLootEntity),
                                                 itemDb, uiFont, textRenderer);
            }

            // Hover highlight / drop-preview — while dragging, shows
            // whether a drop here would succeed (green) or not (red),
            // using DragDropController::previewDrop() so this can
            // never show something different from what actually
            // happens on release (see DragDropController.h). Otherwise
            // just highlights whatever's under the cursor — a whole
            // stack's footprint, or a single empty cell.
            auto renderPanelHighlight = [&](const engine::ui::GridLayout& layout, game::components::Inventory& inv) {
                auto cell = layout.cellAt(mouseX, mouseY);
                if (!cell) return;
                int hoverX = cell->first, hoverY = cell->second;

                if (dragDrop.isDragging()) {
                    const auto& heldStack = dragDrop.held().stack;
                    const game::data::ItemDefinition* def = itemDb.find(heldStack.itemId);
                    if (!def) return;
                    int w = heldStack.rotated ? def->height : def->width;
                    int h = heldStack.rotated ? def->width : def->height;
                    auto [resolvedX, resolvedY] = dragDrop.resolveDropTopLeft(hoverX, hoverY);
                    bool valid =
                        dragDrop.previewDrop(itemDb, inv, resolvedX, resolvedY) != game::ui::DropOutcome::Invalid;
                    engine::render::Color highlightColor =
                        valid ? engine::render::Color{80, 200, 90, 90} : engine::render::Color{210, 60, 60, 90};
                    engine::ui::renderHighlightRect(renderer, layout, resolvedX, resolvedY, w, h, highlightColor);
                } else if (auto idx = game::systems::InventorySystem::stackIndexAt(inv, itemDb, hoverX, hoverY)) {
                    const auto& s = inv.stacks[*idx];
                    const game::data::ItemDefinition* def = itemDb.find(s.itemId);
                    int w = def ? (s.rotated ? def->height : def->width) : 1;
                    int h = def ? (s.rotated ? def->width : def->height) : 1;
                    engine::ui::renderHighlightRect(renderer, layout, s.gridX, s.gridY, w, h,
                                                     engine::render::Color{255, 255, 255, 55});
                    game::ui::renderItemNameTooltip(renderer, s.itemId, itemDb, uiFont, textRenderer, mouseX, mouseY);
                } else {
                    engine::ui::renderHighlightRect(renderer, layout, hoverX, hoverY, 1, 1,
                                                     engine::render::Color{255, 255, 255, 35});
                }
            };
            renderPanelHighlight(playerPanelLayout, registry.get<game::components::Inventory>(player));
            if (nearbyLootEntity != engine::ecs::kNullEntity && registry.has<game::components::Inventory>(nearbyLootEntity)) {
                renderPanelHighlight(lootPanelLayout, registry.get<game::components::Inventory>(nearbyLootEntity));
            }

            game::ui::renderHeldStack(renderer, itemDb, dragDrop, playerPanelLayout.cellSize, mouseX, mouseY);
        }

        // Hotbar — always visible, unlike the grid panels above, same
        // as any action game's quick-slot bar staying on screen during
        // normal play, not just while managing inventory.
        game::ui::renderHotbar(window.renderer(), hotbarLayout, playerQuickSlots,
                                registry.get<game::components::Inventory>(player), itemDb, uiFont, textRenderer);

        // Custom cursor, drawn last so it's always on top — matches
        // where the OS's own hardware cursor would render.
        cursor.render(window.renderer(), mouseX, mouseY);

        // Application::run() calls window.present() after this callback —
        // don't call it again here.
    });

    app.run();
    return 0;
}
