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
#include "Tiled2dMapVectorLayerConfig.h"
#include "Tiled2dMapZoomInfo.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "Logger.h"
#include "Tiled2dMapVectorSettings.h"
#include "Epsg4326Tiled2dMapLayerConfig.h"

class Tiled2dMapVectorRasterSubLayerConfig : public Tiled2dMapVectorLayerConfig {
public:
    Tiled2dMapVectorRasterSubLayerConfig(const std::shared_ptr<RasterVectorLayerDescription> &layerDescription,
                                         const std::optional<Tiled2dMapZoomInfo> &customZoomInfo = std::nullopt)
    : Tiled2dMapVectorLayerConfig(
                                  std::make_shared<VectorMapSourceDescription>(layerDescription->source, layerDescription->url,
                                                                               layerDescription->minZoom, layerDescription->maxZoom, layerDescription->bounds)),
    description(layerDescription) {
        if (customZoomInfo.has_value()) {
            zoomInfo = Tiled2dMapZoomInfo(customZoomInfo->zoomLevelScaleFactor * description->zoomLevelScaleFactor,
                                          std::max(customZoomInfo->numDrawPreviousLayers, description->numDrawPreviousLayers),
                                          0,
                                          customZoomInfo->adaptScaleToScreen || description->adaptScaleToScreen,
                                          customZoomInfo->maskTile || description->maskTiles,
                                          customZoomInfo->underzoom && description->underzoom,
                                          customZoomInfo->overzoom && description->overzoom);
        } else {
            zoomInfo = Tiled2dMapZoomInfo(description->zoomLevelScaleFactor, description->numDrawPreviousLayers, 0,
                                          description->adaptScaleToScreen, description->maskTiles, description->underzoom,
                                          description->overzoom);
        }

        if (description->coordinateReferenceSystem == "EPSG:4326") {
            customConfig = std::make_shared<Epsg4326Tiled2dMapLayerConfig>(layerDescription->source, layerDescription->url, zoomInfo, layerDescription->minZoom, layerDescription->maxZoom);
        }
    }

    int32_t getCoordinateSystemIdentifier() override {
        if (customConfig) {
            return customConfig->getCoordinateSystemIdentifier();
        }
        return Tiled2dMapVectorLayerConfig::getCoordinateSystemIdentifier();
    }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        if (customConfig) {
            return customConfig->getTileUrl(x,y,t,zoom);
        }
        return Tiled2dMapVectorLayerConfig::getTileUrl(x,y,t,zoom);
    }

    std::string getLayerName() override {
        if (customConfig) {
            return customConfig->getLayerName();
        }
        return Tiled2dMapVectorLayerConfig::getLayerName();
    }

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override {
        if (customConfig) {
            return customConfig->getZoomLevelInfos();
        }
        return Tiled2dMapVectorLayerConfig::getZoomLevelInfos();
    }

    Tiled2dMapZoomInfo getZoomInfo() override {
        if (customConfig) {
            return customConfig->getZoomInfo();
        }
        return Tiled2dMapVectorLayerConfig::getZoomInfo();
    }

    std::optional<Tiled2dMapVectorSettings> getVectorSettings() override {
        if (customConfig) {
            return customConfig->getVectorSettings();
        }
        return Tiled2dMapVectorLayerConfig::getVectorSettings();
    }

    std::optional<::RectCoord> getBounds() override {
        if (customConfig) {
            return customConfig->getBounds();
        }
        return Tiled2dMapVectorLayerConfig::getBounds();
    }


private:
    std::shared_ptr<RasterVectorLayerDescription> description;

    std::shared_ptr<Tiled2dMapLayerConfig> customConfig;
};
