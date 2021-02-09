//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once

struct Tiled2dMapTileInfo {
    int x;
    int y;
    int zoom;
    int loadingPriority; // priorities > 0, if must be loaded

    Tiled2dMapTileInfo(int x, int y, int zoom, int loadingPriority) : x(x), y(y), zoom(zoom), loadingPriority(loadingPriority) {}

    bool operator==(const Tiled2dMapTileInfo &o) const {
        return x == o.x && y == o.y && zoom == o.zoom && loadingPriority == o.loadingPriority;
    }

    bool operator<(const Tiled2dMapTileInfo &o) const {
        return x < o.x || (x == o.x && y < o.y) || (x == o.x && y == o.y && zoom < o.zoom) ||
               (x == o.x && y == o.y && zoom == o.zoom && loadingPriority < o.loadingPriority);
    }
};
