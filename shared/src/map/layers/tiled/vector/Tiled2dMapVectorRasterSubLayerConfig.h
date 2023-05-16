/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */
#pragma once

#include "VectorLayerDescription.h"
#include "RasterVectorLayerDescription.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapZoomInfo.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "Logger.h"
#include "Tiled2dMapVectorSettings.h"

class Tiled2dMapVectorRasterSubLayerConfig : public Tiled2dMapLayerConfig {
public:
    Tiled2dMapVectorRasterSubLayerConfig(const std::shared_ptr<RasterVectorLayerDescription> &layerDescription)
            : description(layerDescription) {}

    ~Tiled2dMapVectorRasterSubLayerConfig() {}

    std::string getCoordinateSystemIdentifier() override {
        return epsg3857Id;
    }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        std::string url = description->url;
        size_t zoomIndex = url.find("{z}", 0);
        if (zoomIndex == std::string::npos)
            throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
        url = url.replace(zoomIndex, 3, std::to_string(zoom));
        size_t xIndex = url.find("{x}", 0);
        if (xIndex == std::string::npos)
            throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
        url = url.replace(xIndex, 3, std::to_string(x));
        size_t yIndex = url.find("{y}", 0);
        if (yIndex == std::string::npos)
            throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
        return url.replace(yIndex, 3, std::to_string(y));
    }

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override {
        return getDefaultEpsg3857ZoomLevels();
    }

    Tiled2dMapZoomInfo getZoomInfo() override {
        return Tiled2dMapZoomInfo(description->zoomLevelScaleFactor, description->numDrawPreviousLayers,
                                  description->adaptScaleToScreen, description->maskTiles, description->underzoom,
                                  description->overzoom);
    }

    static double getZoomIdentifier(double zoom) {
        return log(Tiled2dMapVectorRasterSubLayerConfig::baseValueZoom / zoom) / log(2);
    }

    std::string getLayerName() override {
        return description->identifier;
    }

    std::optional<Tiled2dMapVectorSettings> getVectorSettings() override {
        return std::nullopt;
    }

    static constexpr double getZoomFactorAtIdentifier(double zoomIdentifier) {
        double factor = pow(2, zoomIdentifier);
        return Tiled2dMapVectorRasterSubLayerConfig::baseValueZoom / factor;
    }

private:
    std::shared_ptr<RasterVectorLayerDescription> description;

    static constexpr double baseValueZoom = 500000000.0;
    const double baseValueWidth = 40075016.0;
    const std::string epsg3857Id = CoordinateSystemIdentifiers::EPSG3857();
    const RectCoord epsg3857Bounds = RectCoord(
            Coord(epsg3857Id, -20037508.34, 20037508.34, 0.0),
            Coord(epsg3857Id, 20037508.34, -20037508.34, 0.0)
    );

    virtual std::vector<Tiled2dMapZoomLevelInfo> getDefaultEpsg3857ZoomLevels() {
        std::vector<Tiled2dMapZoomLevelInfo> infos;
        for (int i = description->minZoom; i <= description->maxZoom; i++) {
            double factor = pow(2, i);
            double zoom = Tiled2dMapVectorRasterSubLayerConfig::baseValueZoom / factor;
            double width = baseValueWidth / factor;
            infos.push_back(Tiled2dMapZoomLevelInfo(zoom, width, factor, factor, 1, i, epsg3857Bounds));
        }
        return infos;
    }
};
