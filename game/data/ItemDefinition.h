#pragma once

#include <optional>
#include <string>

#include "render/Color.h"

namespace game::data {

// Which combat mechanic a weapon item uses — see game::systems::
// CombatSystem (Ranged, hitscan) vs MeleeCombatSystem (Melee,
// swing/arc). Only meaningful when an ItemDefinition::weaponStats is
// present.
enum class WeaponKind { Ranged, Melee };

// Combat stats for a weapon item — present only on items that are
// actually weapons (category == "weapon" by convention, but what
// actually gates behavior is whether weaponStats is set, not the
// category string). A flat struct with both ranged and melee fields
// rather than two separate types: only the fields matching `kind` are
// ever read (see EquipmentSystem::syncActiveWeapon), and this stays a
// single plain-data JSON record instead of needing a tagged union or
// inheritance for what's still a small, fixed set of fields. Mirrors
// game::components::Weapon/MeleeWeapon field-for-field, since equipping
// an item is exactly "copy these into whichever component is active."
struct WeaponStats {
    WeaponKind kind = WeaponKind::Ranged;

    // Ranged (WeaponKind::Ranged) — see game::components::Weapon for
    // what each field means.
    float fireRate = 6.0f;
    float damage = 25.0f;
    float range = 800.0f;
    float spreadDegrees = 2.0f;
    float adsZoomMultiplier = 1.15f;
    float adsOffsetPixels = 50.0f;

    // Melee (WeaponKind::Melee) — see game::components::MeleeWeapon.
    float meleeDamage = 30.0f;
    float meleeRange = 55.0f;
    float meleeArcDegrees = 90.0f;
    float meleeAttacksPerSecond = 2.0f;
};

// A consumable item's effect when used from a hotbar quick slot — see
// QuickSlots/UseItemSystem. Just a heal amount for now; food/water
// restoration would extend this once Milestone 9's survival stats
// exist, not attempted here.
struct UseEffect {
    float healAmount = 0.0f;
};

// One item *type*'s static data — loaded from assets/items/items.json,
// never mutated at runtime. An Inventory stack references one of these
// by id; the stack itself (game/components/Inventory.h) holds the
// per-instance quantity and grid position.
struct ItemDefinition {
    std::string id;
    std::string name;

    // Footprint in grid cells (Tarkov/Zero-Sievert-style shaped
    // inventory — confirmed as the intended model, not simple 1-slot
    // stacking). Both 1 for a small item like a bandage; a rifle might
    // be {2, 4}. No rotation support in the item *definition* itself —
    // rotation is a per-placed-stack, drag-time-only property (see
    // InventoryStack::rotated).
    int width = 1;
    int height = 1;

    // How many can occupy a single stack (a single grid placement) —
    // 1 means non-stackable (every unit needs its own grid placement).
    int maxStack = 1;

    float weight = 1.0f;    // per unit, kg — see InventorySystem's weight limit
    std::string category;   // "weapon", "ammo", "medical", "misc", ... — free-form for now

    // Real icon art is later content; this is the same colored-rectangle
    // placeholder convention every other visual in this codebase uses
    // until then (engine::render::SpriteRenderSystem already falls back
    // to a tinted rectangle when Sprite::texture is null).
    engine::render::Color iconColor{200, 200, 200, 255};

    // Set only for items that are actually equippable weapons — see
    // WeaponStats above and EquipmentSystem::equip, which refuses to
    // equip an item that doesn't have this set (or whose kind doesn't
    // match the target slot).
    std::optional<WeaponStats> weaponStats;

    // Set only for backpack/gear items — how much carry-weight budget
    // equipping this into EquipmentSlots::Slot::Backpack grants. See
    // EquipmentSystem's capacity constants for how this becomes the
    // player's actual soft/hard weight caps.
    std::optional<float> carryCapacityKg;

    // Set only for consumable items usable from a hotbar quick slot
    // (see QuickSlots/UseItemSystem) — a bandage restoring Health, for
    // instance. Deliberately just a heal amount for now, not a
    // duration/channel time — see UseItemSystem's own note on why an
    // instant-use v1 is a reasonable simplification here.
    std::optional<UseEffect> useEffect;
};

} // namespace game::data
