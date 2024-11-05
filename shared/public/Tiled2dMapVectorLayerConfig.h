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

#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "VectorMapSourceDescription.h"
#include "VectorLayerDescription.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapVectorSettings.h"
#include "Logger.h"

class Tiled2dMapVectorLayerConfig : public Tiled2dMapLayerConfig {
public:
    Tiled2dMapVectorLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &sourceDescription,
                                const Tiled2dMapZoomInfo &zoomInfo)
            : sourceDescription(sourceDescription), zoomInfo(zoomInfo) {}

    Tiled2dMapVectorLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &sourceDescription,
                                const bool is3d)
            : sourceDescription(sourceDescription),
              zoomInfo(Tiled2dMapZoomInfo(
                      sourceDescription->zoomLevelScaleFactor ? *sourceDescription->zoomLevelScaleFactor : (is3d ? 0.75 : 1.0),
                      sourceDescription->numDrawPreviousLayers ? *sourceDescription->numDrawPreviousLayers : 0,
                      0,
                      sourceDescription->adaptScaleToScreen ? *sourceDescription->adaptScaleToScreen : false,
                      true,
                      sourceDescription->underzoom ? *sourceDescription->underzoom : false,
                      sourceDescription->overzoom ? *sourceDescription->overzoom : true)) {}

    ~Tiled2dMapVectorLayerConfig() {}

    int32_t getCoordinateSystemIdentifier() override {
        return epsg3857Id;
    }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        std::string url = sourceDescription->vectorUrl;
        size_t epsg3857Index = url.find("{bbox-epsg-3857}", 0);
        if (epsg3857Index != std::string::npos) {
            const auto zoomLevelInfos = getDefaultEpsg3857ZoomLevels(zoom, zoom);
            const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(0);
            RectCoord layerBounds = zoomLevelInfo.bounds;
            const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

            const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
            const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
            const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
            const double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

            const double boundsLeft = layerBounds.topLeft.x;
            const double boundsTop = layerBounds.topLeft.y;

            const Coord topLeft = Coord(epsg3857Id, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
            const Coord bottomRight = Coord(epsg3857Id, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

            std::string boxString = std::to_string(topLeft.x) + "," + std::to_string(bottomRight.y) + "," + std::to_string(bottomRight.x) + "," + std::to_string(topLeft.y);
            url = url.replace(epsg3857Index, 16, boxString);
            return url;

        }
        size_t zoomIndex = url.find("{z}", 0);
        if (zoomIndex == std::string::npos) throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
        url = url.replace(zoomIndex, 3, std::to_string(zoom));
        size_t xIndex = url.find("{x}", 0);
        if (xIndex == std::string::npos) throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
        url = url.replace(xIndex, 3, std::to_string(x));
        size_t yIndex = url.find("{y}", 0);
        if (yIndex == std::string::npos) throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
        return url.replace(yIndex, 3, std::to_string(y));
    }

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override {
        return getDefaultEpsg3857ZoomLevels(sourceDescription->minZoom, sourceDescription->maxZoom);
    }

    std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override {
        return getDefaultEpsg3857ZoomLevels(0, sourceDescription->minZoom - 1);
    };

    Tiled2dMapZoomInfo getZoomInfo() override {
        return zoomInfo;
    }

    std::string getLayerName() override {
        return sourceDescription->identifier;
    }

    std::optional<Tiled2dMapVectorSettings> getVectorSettings() override {
        return std::nullopt;
    }

    virtual double getZoomIdentifier(double zoom) {
        return std::max(0.0, std::round(log(baseValueZoom * zoomInfo.zoomLevelScaleFactor / zoom) / log(2) * 100) / 100);
    }

    virtual double getZoomFactorAtIdentifier(double zoomIdentifier) {
        double factor = pow(2, zoomIdentifier);
        return baseValueZoom * zoomInfo.zoomLevelScaleFactor / factor;

    }

    std::optional<::RectCoord> getBounds() override {
        if (sourceDescription) {
            return sourceDescription->bounds;
        }
        else {
            return std::nullopt;
        }
    }

    static std::vector<Tiled2dMapZoomLevelInfo> getDefaultEpsg3857ZoomLevels(int minZoom, int maxZoom) {
        std::vector<Tiled2dMapZoomLevelInfo> infos;
        for (int i = minZoom; i <= maxZoom; i++) {
            double factor = pow(2, i);
            double zoom = baseValueZoom / factor;
            double width = baseValueWidth / factor;
            infos.push_back(Tiled2dMapZoomLevelInfo(zoom, width, factor, factor, 1, i, epsg3857Bounds));
        }
        return infos;
    }


protected:
    std::shared_ptr<VectorMapSourceDescription> sourceDescription;
    Tiled2dMapZoomInfo zoomInfo;

    static constexpr double baseValueZoom = 500000000.0;
    static constexpr double baseValueWidth = 40075016.0;
    static const inline int32_t epsg3857Id = CoordinateSystemIdentifiers::EPSG3857();
    static const inline RectCoord epsg3857Bounds = RectCoord(
            Coord(epsg3857Id, -20037508.34, 20037508.34, 0.0),
            Coord(epsg3857Id, 20037508.34, -20037508.34, 0.0)
    );


};
