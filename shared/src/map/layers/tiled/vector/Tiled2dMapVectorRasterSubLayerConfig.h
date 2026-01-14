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
                                         const bool is3d,
                                         const std::optional<Tiled2dMapZoomInfo> &customZoomInfo = std::nullopt)
            : Tiled2dMapVectorLayerConfig(std::static_pointer_cast<VectorMapSourceDescription>(layerDescription->source), is3d),
              description(layerDescription) {
        if (customZoomInfo.has_value()) {
            // zoomInfo is already initialized in super from the source-description
            zoomInfo.zoomLevelScaleFactor *= customZoomInfo->zoomLevelScaleFactor;
            zoomInfo.numDrawPreviousLayers = std::max(customZoomInfo->numDrawPreviousLayers, zoomInfo.numDrawPreviousLayers);
            zoomInfo.numDrawPreviousOrLaterTLayers = 0;
            zoomInfo.adaptScaleToScreen |= customZoomInfo->adaptScaleToScreen;
            zoomInfo.maskTile |= customZoomInfo->maskTile;
            zoomInfo.underzoom &= customZoomInfo->underzoom;
            zoomInfo.overzoom &= customZoomInfo->overzoom;
        }

        if (description->source->coordinateReferenceSystem == "EPSG:4326") {
            customConfig = std::make_shared<Epsg4326Tiled2dMapLayerConfig>(layerDescription->source->identifier,
                                                                           layerDescription->source->vectorUrl,
                                                                           zoomInfo,
                                                                           layerDescription->sourceMinZoom,
                                                                           layerDescription->sourceMaxZoom);
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

    std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override {
        if (customConfig) {
            return customConfig->getVirtualZoomLevelInfos();
        }
        return Tiled2dMapVectorLayerConfig::getVirtualZoomLevelInfos();
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
