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
#include "CoordinateSystemFactory.h"
#include "MapCoordinateSystem.h"
#include "Tiled2dMapVectorSettings.h"
#include "Logger.h"
#include <cmath>
#include <stdexcept>

class Tiled2dMapVectorLayerConfig : public Tiled2dMapLayerConfig {
public:
    Tiled2dMapVectorLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &sourceDescription,
                                const Tiled2dMapZoomInfo &zoomInfo)
            : sourceDescription(sourceDescription),
              zoomInfo(zoomInfo),
              mapCoordinateSystem(resolveMapCoordinateSystem(sourceDescription)) {}

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
                      sourceDescription->overzoom ? *sourceDescription->overzoom : true)),
              mapCoordinateSystem(resolveMapCoordinateSystem(sourceDescription)) {}

    ~Tiled2dMapVectorLayerConfig() {}

    int32_t getCoordinateSystemIdentifier() override {
        return mapCoordinateSystem.identifier;
    }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        std::string url = sourceDescription->vectorUrl;
        const std::string bboxPrefix = "{bbox-epsg-";
        size_t bboxIndex = url.find(bboxPrefix, 0);
        if (bboxIndex != std::string::npos) {
            size_t bboxEndIndex = url.find("}", bboxIndex);
            if (bboxEndIndex != std::string::npos) {
                const auto coordinateSystemIdentifier = std::stoi(url.substr(bboxIndex + bboxPrefix.size(),
                                                                             bboxEndIndex - bboxIndex - bboxPrefix.size()));
                const auto zoomLevelInfos = getDefaultZoomLevels(getCoordinateSystem(coordinateSystemIdentifier), zoom, zoom, std::nullopt);
                const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(0);
                RectCoord layerBounds = zoomLevelInfo.bounds;
                const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

                const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
                const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
                const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
                const double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

                const double boundsLeft = layerBounds.topLeft.x;
                const double boundsTop = layerBounds.topLeft.y;

                const Coord topLeft = Coord(coordinateSystemIdentifier, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                const Coord bottomRight = Coord(coordinateSystemIdentifier, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

                std::string boxString = std::to_string(topLeft.x) + "," + std::to_string(bottomRight.y) + "," +
                                        std::to_string(bottomRight.x) + "," + std::to_string(topLeft.y);
                url = url.replace(bboxIndex, bboxEndIndex - bboxIndex + 1, boxString);
                return url;
            }
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
        return getDefaultZoomLevels(mapCoordinateSystem, sourceDescription->minZoom, sourceDescription->maxZoom, sourceDescription->levels);
    }

    std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override {
        return getDefaultZoomLevels(mapCoordinateSystem, 0, sourceDescription->minZoom - 1, sourceDescription->levels);
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

    static std::vector<Tiled2dMapZoomLevelInfo> getDefaultZoomLevels(const MapCoordinateSystem &coordinateSystem,
                                                                     int minZoom,
                                                                     int maxZoom,
                                                                     const std::optional<std::vector<int>> &levels) {
        std::vector<Tiled2dMapZoomLevelInfo> infos;
        const double baseWidth = std::abs(coordinateSystem.bounds.bottomRight.x - coordinateSystem.bounds.topLeft.x);
        if (levels.has_value()) {
            for (const auto &level : levels.value()) {
                double factor = pow(2, level);
                double zoom = baseValueZoom / factor;
                double width = baseWidth / factor;
                infos.push_back(Tiled2dMapZoomLevelInfo(zoom, width, factor, factor, 1, level, coordinateSystem.bounds));
            }
        } else {
            for (int i = minZoom; i <= maxZoom; i++) {
                double factor = pow(2, i);
                double zoom = baseValueZoom / factor;
                double width = baseWidth / factor;
                infos.push_back(Tiled2dMapZoomLevelInfo(zoom, width, factor, factor, 1, i, coordinateSystem.bounds));
            }
        }
        return infos;
    }

    static std::vector<Tiled2dMapZoomLevelInfo> getDefaultEpsg3857ZoomLevels(int minZoom, int maxZoom, const std::optional<std::vector<int>> &levels) {
        return getDefaultZoomLevels(CoordinateSystemFactory::getEpsg3857System(), minZoom, maxZoom, levels);
    }

protected:
    std::shared_ptr<VectorMapSourceDescription> sourceDescription;
    Tiled2dMapZoomInfo zoomInfo;
    MapCoordinateSystem mapCoordinateSystem;

    static constexpr double baseValueZoom = 500000000.0;

    static MapCoordinateSystem resolveMapCoordinateSystem(const std::shared_ptr<VectorMapSourceDescription> &sourceDescription) {
        if (sourceDescription && sourceDescription->coordinateReferenceSystem.has_value()) {
            try {
                const auto coordinateSystemIdentifier =
                    CoordinateSystemIdentifiers::fromCrsIdentifier(*sourceDescription->coordinateReferenceSystem);
                return getCoordinateSystem(coordinateSystemIdentifier);
            } catch (const std::invalid_argument &ex) {
                LogError <<= "Unsupported CRS " + *sourceDescription->coordinateReferenceSystem + ": " + ex.what();
            }
        }
        return CoordinateSystemFactory::getEpsg3857System();
    }

    static MapCoordinateSystem getCoordinateSystem(int32_t coordinateSystemIdentifier) {
        if (coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG3857()) {
            return CoordinateSystemFactory::getEpsg3857System();
        }
        if (coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG4326()) {
            return CoordinateSystemFactory::getEpsg4326System();
        }
        if (coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG2056()) {
            return CoordinateSystemFactory::getEpsg2056System();
        }
        if (coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG21781()) {
            return CoordinateSystemFactory::getEpsg21781System();
        }
        throw std::invalid_argument("Unsupported coordinate system identifier: " + std::to_string(coordinateSystemIdentifier));
    }

};
