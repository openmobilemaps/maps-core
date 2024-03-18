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
    }

private:
    std::shared_ptr<RasterVectorLayerDescription> description;
};
