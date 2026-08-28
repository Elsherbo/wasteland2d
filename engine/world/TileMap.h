#pragma once

#include <SDL.h>
#include <string>
#include <vector>

#include "physics/AABB.h"
#include "render/Camera.h"
#include "resource/TextureCache.h"

namespace engine::world {

// Loads and renders a Tiled (mapeditor.org) .tmx map.
//
// Supported subset — deliberately narrow to avoid pulling in zlib/base64
// decompression or a second .tsx-parsing path for a framework this size:
//   - ONE tileset, embedded directly in the .tmx (Tiled: tileset ->
//     "Embed in map"), no margin/spacing between tiles.
//   - Tile layers encoded as CSV (Tiled: layer properties -> Format:
//     "CSV"), not Base64/zlib/gzip.
//   - Collision comes from a single object layer named "collision"
//     containing plain rectangle objects — not from per-tile collision
//     shapes. This matches how most people actually block out top-down
//     level collision in Tiled and keeps the parser simple.
// If a future map needs more than this (external tilesets, compressed
// layers, multiple tilesets), extend TileMap.cpp rather than reaching
// for a full tmxlite-style dependency — until then this covers every
// milestone through at least the AI/loot passes.
class TileMap {
public:
    // textures must outlive this TileMap — the tileset image is loaded
    // (and cached) through it (Milestone 5.5), rather than TileMap
    // loading and owning its own SDL_Texture the way it did through
    // Milestone 4. This makes TextureCache the single texture-owning
    // path in the engine.
    TileMap(resource::TextureCache& textures, const std::string& tmxPath);
    ~TileMap();

    TileMap(const TileMap&) = delete;
    TileMap& operator=(const TileMap&) = delete;

    void render(SDL_Renderer* renderer, const render::Camera& camera) const;

    // True if box overlaps any collision rectangle from the "collision"
    // object layer. Linear scan — fine for the rect counts a hand-built
    // level has; revisit with a spatial grid if that ever changes.
    // Kept for callers that want a quick AABB check without touching
    // physics at all; as of Milestone 4, actual gameplay collision goes
    // through PhysicsWorld::createStaticBodiesFromRects(colliders())
    // instead of calling this directly every frame.
    bool collides(const physics::AABB& box) const;

    // Raw collision rectangles from the "collision" object layer, in
    // world pixel coordinates — feed these to
    // PhysicsWorld::createStaticBodiesFromRects() once, at map load.
    const std::vector<physics::AABB>& colliders() const { return colliders_; }

    int widthTiles() const { return widthTiles_; }
    int heightTiles() const { return heightTiles_; }
    int tileWidth() const { return tileWidth_; }
    int tileHeight() const { return tileHeight_; }
    int widthPixels() const { return widthTiles_ * tileWidth_; }
    int heightPixels() const { return heightTiles_ * tileHeight_; }

private:
    struct TileLayer {
        std::string name;
        std::vector<int> gids; // row-major, size = widthTiles_ * heightTiles_, 0 = empty
    };

    int widthTiles_ = 0;
    int heightTiles_ = 0;
    int tileWidth_ = 0;
    int tileHeight_ = 0;

    SDL_Texture* tilesetTexture_ = nullptr;
    int tilesetColumns_ = 0;
    int tilesetFirstGid_ = 1;

    std::vector<TileLayer> layers_;
    std::vector<physics::AABB> colliders_;
};

} // namespace engine::world
