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

#include "GeoJsonTypes.h"
#include "RectCoord.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "Tiled2dMapZoomInfo.h"
#include "VectorLayerDescription.h"

class VectorMapSourceDescription {
public:
    std::string identifier;
    std::string vectorUrl;
    int minZoom;
    int maxZoom;
    std::optional<::RectCoord> bounds;
    std::optional<bool> adaptScaleToScreen;
    std::optional<int> numDrawPreviousLayers;
    std::optional<float> zoomLevelScaleFactor;
    std::optional<bool> underzoom;
    std::optional<bool> overzoom;
    std::optional<std::vector<int>> levels;
    std::optional<int32_t> coordinateReferenceSystem;

    VectorMapSourceDescription(std::string identifier,
                               std::string vectorUrl,
                               int minZoom,
                               int maxZoom,
                               std::optional<::RectCoord> bounds,
                               std::optional<float> zoomLevelScaleFactor,
                               std::optional<bool> adaptScaleToScreen,
                               std::optional<int> numDrawPreviousLayers,
                               std::optional<bool> underzoom,
                               std::optional<bool> overzoom,
                               std::optional<std::vector<int>> levels,
                               std::optional<int32_t> coordinateReferenceSystem) :
            identifier(identifier), vectorUrl(vectorUrl), minZoom(minZoom), maxZoom(maxZoom), bounds(bounds),
            adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers),
            zoomLevelScaleFactor(zoomLevelScaleFactor), underzoom(underzoom), overzoom(overzoom), levels(levels),
            coordinateReferenceSystem(coordinateReferenceSystem) {}

    virtual Tiled2dMapZoomInfo getZoomInfo(bool is3d) const {
        return Tiled2dMapZoomInfo(
                zoomLevelScaleFactor ? *zoomLevelScaleFactor : (is3d ? 0.75 : 1.0),
                numDrawPreviousLayers ? *numDrawPreviousLayers : 0,
                0,
                adaptScaleToScreen ? *adaptScaleToScreen : false,
                true,
                underzoom ? *underzoom : false,
                overzoom ? *overzoom : true);
    };

		std::vector<int> getZoomLevels() {
				if(levels) {
						return *levels;
				} else {
						return Tiled2dMapVectorLayerConfig::generateLevelsFromMinMax(minZoom, maxZoom);
				}
		}
};

struct SpriteSourceDescription {
    std::string identifier;
    std::string baseUrl;

    SpriteSourceDescription(std::string identifier, std::string baseUrl)
        : identifier(identifier)
        , baseUrl(baseUrl) {}
};

class VectorMapDescription {
public:
    std::string identifier;
    std::vector<std::shared_ptr<VectorMapSourceDescription>> vectorSources;
    std::vector<std::shared_ptr<VectorLayerDescription>> layers;
    std::vector<SpriteSourceDescription> sprites;
    std::map<std::string, std::shared_ptr<GeoJSONVTInterface>> geoJsonSources;
    bool persistingSymbolPlacement;
    std::optional<bool> use3xSprites;

    VectorMapDescription(std::string identifier,
                         std::vector<std::shared_ptr<VectorMapSourceDescription>> vectorSources,
                         std::vector<std::shared_ptr<VectorLayerDescription>> layers,
                         std::vector<SpriteSourceDescription> sprites,
                         std::map<std::string, std::shared_ptr<GeoJSONVTInterface>> geoJsonSources,
                         bool persistingSymbolPlacement,
                         std::optional<bool> use3xSprites)
        : identifier(std::move(identifier))
        , vectorSources(std::move(vectorSources))
        , layers(std::move(layers))
        , sprites(std::move(sprites))
        , geoJsonSources(std::move(geoJsonSources))
        , persistingSymbolPlacement(persistingSymbolPlacement)
        , use3xSprites(use3xSprites) {}
};
