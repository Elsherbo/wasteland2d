#include "data/ItemDatabase.h"

#include <cstdint>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace game::data {

void ItemDatabase::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open item database: " + path);
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Malformed item database JSON (" + path + "): " + e.what());
    }

    if (!root.is_array()) {
        throw std::runtime_error("Item database must be a JSON array of item records: " + path);
    }

    for (const auto& record : root) {
        ItemDefinition item;
        try {
            item.id = record.at("id").get<std::string>();
            item.name = record.at("name").get<std::string>();
            item.width = record.value("width", 1);
            item.height = record.value("height", 1);
            item.maxStack = record.value("maxStack", 1);
            item.weight = record.value("weight", 1.0f);
            item.category = record.value("category", std::string("misc"));

            if (record.contains("iconColor")) {
                const auto& c = record.at("iconColor");
                item.iconColor = engine::render::Color{
                    static_cast<std::uint8_t>(c.value("r", 200)),
                    static_cast<std::uint8_t>(c.value("g", 200)),
                    static_cast<std::uint8_t>(c.value("b", 200)),
                    static_cast<std::uint8_t>(c.value("a", 255))};
            }

            if (record.contains("weaponStats")) {
                const auto& w = record.at("weaponStats");
                WeaponStats stats;

                std::string kindStr = w.value("kind", std::string("ranged"));
                if (kindStr == "ranged") {
                    stats.kind = WeaponKind::Ranged;
                } else if (kindStr == "melee") {
                    stats.kind = WeaponKind::Melee;
                } else {
                    throw std::runtime_error("Item '" + item.id + "' has unknown weaponStats.kind '" + kindStr +
                                              "' (expected 'ranged' or 'melee'): " + path);
                }

                stats.fireRate = w.value("fireRate", stats.fireRate);
                stats.damage = w.value("damage", stats.damage);
                stats.range = w.value("range", stats.range);
                stats.spreadDegrees = w.value("spreadDegrees", stats.spreadDegrees);
                stats.adsZoomMultiplier = w.value("adsZoomMultiplier", stats.adsZoomMultiplier);
                stats.adsOffsetPixels = w.value("adsOffsetPixels", stats.adsOffsetPixels);

                stats.meleeDamage = w.value("meleeDamage", stats.meleeDamage);
                stats.meleeRange = w.value("meleeRange", stats.meleeRange);
                stats.meleeArcDegrees = w.value("meleeArcDegrees", stats.meleeArcDegrees);
                stats.meleeAttacksPerSecond = w.value("meleeAttacksPerSecond", stats.meleeAttacksPerSecond);

                item.weaponStats = stats;
            }

            if (record.contains("carryCapacityKg")) {
                item.carryCapacityKg = record.at("carryCapacityKg").get<float>();
            }

            if (record.contains("useEffect")) {
                const auto& u = record.at("useEffect");
                UseEffect effect;
                effect.healAmount = u.value("healAmount", effect.healAmount);
                item.useEffect = effect;
            }
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Malformed item record in " + path + ": " + e.what());
        }

        if (item.width <= 0 || item.height <= 0) {
            throw std::runtime_error("Item '" + item.id + "' has a non-positive grid footprint: " + path);
        }
        if (item.maxStack <= 0) {
            throw std::runtime_error("Item '" + item.id + "' has maxStack <= 0: " + path);
        }

        auto [it, inserted] = items_.emplace(item.id, item);
        (void)it;
        if (!inserted) {
            throw std::runtime_error("Duplicate item id '" + item.id + "' in " + path);
        }
    }
}

const ItemDefinition* ItemDatabase::find(const std::string& id) const {
    auto it = items_.find(id);
    return it != items_.end() ? &it->second : nullptr;
}

} // namespace game::data
