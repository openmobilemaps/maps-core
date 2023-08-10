#pragma once

#include "convert.hpp"
#include "tile.hpp"
#include "clip.hpp"

#include <chrono>
#include <cmath>
#include <map>
#include <unordered_map>
#include "GeoJsonTypes.h"

struct TileOptions {
    // simplification tolerance (higher means simpler)
    double tolerance = 3;

    // tile extent
    uint16_t extent = 4096;

    // tile buffer on each side
    uint16_t buffer = 64;
};

struct Options : TileOptions {
    // max zoom to preserve detail on
    uint8_t maxZoom = 18;

    // max zoom in the tile index
    uint8_t indexMaxZoom = 5;

    // max number of points per tile in the tile index
    uint32_t indexMaxPoints = 100000;
};

inline uint64_t toID(uint8_t z, uint32_t x, uint32_t y) {
    return (((1ull << z) * y + x) * 32) + z;
}

class GeoJSONVT {
public:
    const Options options;

    const Tile emptyTile = Tile();

    GeoJSONVT(const std::shared_ptr<GeoJson> &geoJson,
              const Options& options_ = Options())
        : options(options_) {

        const uint32_t z2 = 1u << options.maxZoom;

        convert(geoJson->geometries, (options.tolerance / options.extent) / z2);

        splitTile(geoJson->geometries, 0, 0, 0);
    }

    const Tile& getTile(const uint8_t z, const uint32_t x_, const uint32_t y) {
        if (z > options.maxZoom)
            throw std::runtime_error("Requested zoom higher than maxZoom: " + std::to_string(z));

        const uint32_t z2 = 1u << z;
        const uint32_t x = ((x_ % z2) + z2) % z2; // wrap tile x coordinate
        const uint64_t id = toID(z, x, y);

        auto it = tiles.find(id);
        if (it != tiles.end())
            return it->second.tile;

        it = findParent(z, x, y);

        if (it == tiles.end())
            throw std::runtime_error("Parent tile not found");

        // if we found a parent tile containing the original geometry, we can drill down from it
        const auto& parent = it->second;

        // drill down parent tile up to the requested one
        splitTile(parent.source_features, parent.z, parent.x, parent.y, z, x, y);

        it = tiles.find(id);
        if (it != tiles.end())
            return it->second.tile;

        it = findParent(z, x, y);
        if (it == tiles.end())
            throw std::runtime_error("Parent tile not found");

        return emptyTile;
    }

private:
    std::unordered_map<uint64_t, InternalTile> tiles;

    std::unordered_map<uint64_t, InternalTile>::iterator findParent(const uint8_t z, const uint32_t x, const uint32_t y) {
        uint8_t z0 = z;
        uint32_t x0 = x;
        uint32_t y0 = y;

        const auto end = tiles.end();
        auto parent = end;

        while ((parent == end) && (z0 != 0)) {
            z0--;
            x0 = x0 / 2;
            y0 = y0 / 2;
            parent = tiles.find(toID(z0, x0, y0));
        }

        return parent;
    }

    void splitTile(const std::vector<std::shared_ptr<GeoJsonGeometry>> &geometries,
                   const uint8_t z,
                   const uint32_t x,
                   const uint32_t y,
                   const uint8_t cz = 0,
                   const uint32_t cx = 0,
                   const uint32_t cy = 0) {
        
        const double z2 = 1u << z;
        const uint64_t id = toID(z, x, y);
        
        auto it = tiles.find(id);
        
        if (it == tiles.end()) {
            const double tolerance =
            (z == options.maxZoom ? 0 : options.tolerance / (z2 * options.extent));
            it = tiles.emplace(id, InternalTile{ geometries, z, x, y, options.extent, tolerance}).first;
        }
        
        auto& tile = it->second;
        
        if (geometries.empty())
            return;
        
        //if it's the first-pass tiling
        if (cz == 0u) {
            // stop tiling if we reached max zoom, or if the tile is too simple
            if (z == options.indexMaxZoom || tile.tile.num_points <= options.indexMaxPoints) {
                tile.source_features = geometries;
                return;
            }
        } else { // drilldown to a specific tile;
            // stop tiling if we reached base zoom
            if (z == options.maxZoom)
                return;
            
            // stop tiling if it's our target tile zoom
            if (z == cz) {
                tile.source_features = geometries;
                return;
            }
            
            // stop tiling if it's not an ancestor of the target tile
            const double m = 1u << (cz - z);
            if (x != static_cast<uint32_t>(std::floor(cx / m)) ||
                y != static_cast<uint32_t>(std::floor(cy / m))) {
                tile.source_features = geometries;
                return;
            }
        }
        
        const double p = 0.5 * options.buffer / options.extent;
        const auto& min = tile.bboxMin;
        const auto& max = tile.bboxMax;
        
        const auto left = clip<0>(geometries, (x - p) / z2, (x + 0.5 + p) / z2, min.x, max.x);
        
        splitTile(clip<1>(left, (y - p) / z2, (y + 0.5 + p) / z2, min.y, max.y), z + 1, x * 2, y * 2, cz, cx, cy);
        splitTile(clip<1>(left, (y + 0.5 - p) / z2, (y + 1 + p) / z2, min.y, max.y), z + 1, x * 2, y * 2 + 1, cz, cx, cy);
        
        const auto right = clip<0>(geometries, (x + 0.5 - p) / z2, (x + 1 + p) / z2, min.x, max.x);
        
        splitTile(clip<1>(right, (y - p) / z2, (y + 0.5 + p) / z2, min.y, max.y), z + 1, x * 2 + 1, y * 2, cz, cx, cy);
        splitTile(clip<1>(right, (y + 0.5 - p) / z2, (y + 1 + p) / z2, min.y, max.y), z + 1, x * 2 + 1, y * 2 + 1, cz, cx, cy);
        
        // if we sliced further down, no need to keep source geometry
        tile.source_features.clear();
    }
};
