#pragma once

#include <string>
#include <unordered_map>

#include "data/ItemDefinition.h"

namespace game::data {

// Loads item definitions from a JSON file (see assets/items/items.json
// for the format) into a lookup table. Deliberately isolates the
// nlohmann::json dependency to ItemDatabase.cpp — same pattern
// PhysicsWorld.h/.cpp uses for Box2D and TileMap.h/.cpp uses for
// tinyxml2 — so nothing that just needs to *look up* an item pulls in
// the JSON parser's header.
class ItemDatabase {
public:
    // Throws std::runtime_error on a missing file, malformed JSON, a
    // record missing a required field, or a duplicate id — same
    // fail-fast convention TileMap's TMX loading already uses, rather
    // than silently skipping bad entries.
    void loadFromFile(const std::string& path);

    // nullptr if no item with that id was loaded.
    const ItemDefinition* find(const std::string& id) const;

private:
    std::unordered_map<std::string, ItemDefinition> items_;
};

} // namespace game::data
