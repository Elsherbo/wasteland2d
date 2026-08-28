#include "world/TileMap.h"

#include <tinyxml2.h>

#include <cstdlib>
#include <stdexcept>

namespace engine::world {

namespace {

std::string directoryOf(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    return pos == std::string::npos ? "" : path.substr(0, pos + 1);
}

// Tiled writes CSV tile data as newline-separated rows of
// comma-separated GIDs, often with leading/trailing whitespace per
// line. A single tolerant split on any of ", \t\r\n" handles all of
// that without needing to care about the row structure.
std::vector<int> parseCsvGids(const char* text) {
    std::vector<int> gids;
    if (!text) return gids;

    const char* p = text;
    while (*p) {
        while (*p && (*p == ',' || *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
        if (!*p) break;
        char* end = nullptr;
        long value = std::strtol(p, &end, 10);
        if (end == p) break; // no digits found — malformed data, stop rather than loop forever
        gids.push_back(static_cast<int>(value));
        p = end;
    }
    return gids;
}

} // namespace

TileMap::TileMap(resource::TextureCache& textures, const std::string& tmxPath) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(tmxPath.c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Failed to load TMX: " + tmxPath + " (" + doc.ErrorStr() + ")");
    }

    const tinyxml2::XMLElement* mapEl = doc.FirstChildElement("map");
    if (!mapEl) throw std::runtime_error("TMX has no <map> element: " + tmxPath);

    widthTiles_ = mapEl->IntAttribute("width");
    heightTiles_ = mapEl->IntAttribute("height");
    tileWidth_ = mapEl->IntAttribute("tilewidth");
    tileHeight_ = mapEl->IntAttribute("tileheight");

    // --- tileset (embedded only — see TileMap.h for the constraint) ---
    const tinyxml2::XMLElement* tilesetEl = mapEl->FirstChildElement("tileset");
    if (!tilesetEl) throw std::runtime_error("TMX has no <tileset>: " + tmxPath);

    tilesetFirstGid_ = tilesetEl->IntAttribute("firstgid", 1);
    int tsTileWidth = tilesetEl->IntAttribute("tilewidth", tileWidth_);

    const tinyxml2::XMLElement* imageEl = tilesetEl->FirstChildElement("image");
    if (!imageEl || !imageEl->Attribute("source")) {
        throw std::runtime_error(
            "TMX tileset has no embedded <image> — external .tsx tilesets aren't "
            "supported. In Tiled, check 'Embed in map' when creating the tileset: " +
            tmxPath);
    }

    std::string imagePath = directoryOf(tmxPath) + imageEl->Attribute("source");
    // Loaded (and owned) through the shared TextureCache, not IMG_LoadTexture
    // directly — see the constructor's own doc comment in TileMap.h.
    tilesetTexture_ = textures.load(imagePath);

    int imgW = 0, imgH = 0;
    SDL_QueryTexture(tilesetTexture_, nullptr, nullptr, &imgW, &imgH);
    tilesetColumns_ = tsTileWidth > 0 ? imgW / tsTileWidth : 0;
    if (tilesetColumns_ <= 0) {
        // Deliberately not destroying tilesetTexture_ here: TextureCache
        // owns it now, so TileMap never destroys textures itself — see
        // the ~TileMap() note below.
        throw std::runtime_error("Tileset image too small for its tile width: " + imagePath);
    }

    // --- tile layers (CSV only — see TileMap.h) ---
    for (const tinyxml2::XMLElement* layerEl = mapEl->FirstChildElement("layer"); layerEl;
         layerEl = layerEl->NextSiblingElement("layer")) {
        const tinyxml2::XMLElement* dataEl = layerEl->FirstChildElement("data");
        if (!dataEl) continue;

        const char* encoding = dataEl->Attribute("encoding");
        const char* layerName = layerEl->Attribute("name");
        if (!encoding || std::string(encoding) != "csv") {
            throw std::runtime_error(
                "Layer '" + std::string(layerName ? layerName : "?") +
                "' isn't CSV-encoded. In Tiled: layer properties -> Format -> CSV. "
                "(Base64/zlib/gzip aren't supported by this loader.)");
        }

        TileLayer layer;
        layer.name = layerName ? layerName : "";
        layer.gids = parseCsvGids(dataEl->GetText());
        layers_.push_back(std::move(layer));
    }

    // --- collision object layer ---
    for (const tinyxml2::XMLElement* groupEl = mapEl->FirstChildElement("objectgroup"); groupEl;
         groupEl = groupEl->NextSiblingElement("objectgroup")) {
        const char* name = groupEl->Attribute("name");
        if (!name || std::string(name) != "collision") continue;

        for (const tinyxml2::XMLElement* objEl = groupEl->FirstChildElement("object"); objEl;
             objEl = objEl->NextSiblingElement("object")) {
            physics::AABB box;
            box.x = objEl->DoubleAttribute("x");
            box.y = objEl->DoubleAttribute("y");
            box.w = objEl->DoubleAttribute("width");
            box.h = objEl->DoubleAttribute("height");
            colliders_.push_back(box);
        }
    }
}

TileMap::~TileMap() {
    // tilesetTexture_ is owned by the TextureCache it was loaded
    // through (Milestone 5.5) — nothing to destroy here anymore.
}

void TileMap::render(SDL_Renderer* renderer, const render::Camera& camera) const {
    for (const TileLayer& layer : layers_) {
        for (int row = 0; row < heightTiles_; ++row) {
            for (int col = 0; col < widthTiles_; ++col) {
                int gid = layer.gids[static_cast<std::size_t>(row) * widthTiles_ + col];
                if (gid == 0) continue; // empty cell

                int localId = gid - tilesetFirstGid_;
                if (localId < 0) continue; // gid belongs to a tileset we don't have

                SDL_Rect src;
                src.x = (localId % tilesetColumns_) * tileWidth_;
                src.y = (localId / tilesetColumns_) * tileHeight_;
                src.w = tileWidth_;
                src.h = tileHeight_;

                double worldX = col * tileWidth_ + tileWidth_ * 0.5;
                double worldY = row * tileHeight_ + tileHeight_ * 0.5;
                double screenX, screenY;
                camera.worldToScreen(worldX, worldY, screenX, screenY);

                double w = tileWidth_ * camera.zoom();
                double h = tileHeight_ * camera.zoom();

                SDL_FRect dest;
                dest.x = static_cast<float>(screenX - w * 0.5);
                dest.y = static_cast<float>(screenY - h * 0.5);
                dest.w = static_cast<float>(w);
                dest.h = static_cast<float>(h);

                SDL_RenderCopyF(renderer, tilesetTexture_, &src, &dest);
            }
        }
    }
}

bool TileMap::collides(const physics::AABB& box) const {
    for (const physics::AABB& rect : colliders_) {
        if (physics::intersects(box, rect)) return true;
    }
    return false;
}

} // namespace engine::world
