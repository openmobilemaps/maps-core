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
#include "Color.h"
#include "GeoJsonTypes.h"
#include "RectCoord.h"

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
                               std::optional<std::vector<int>> levels) :
            identifier(identifier), vectorUrl(vectorUrl), minZoom(minZoom), maxZoom(maxZoom), bounds(bounds),
            adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers),
            zoomLevelScaleFactor(zoomLevelScaleFactor), underzoom(underzoom), overzoom(overzoom), levels(levels) {}
};

class VectorMapDescription {
public:
    std::string identifier;
    std::vector<std::shared_ptr<VectorMapSourceDescription>> vectorSources;
    std::vector<std::shared_ptr<VectorLayerDescription>> layers;
    std::optional<std::string> spriteBaseUrl;
    std::map<std::string, std::shared_ptr<GeoJSONVTInterface>> geoJsonSources;
    bool persistingSymbolPlacement;
    std::optional<bool> use3xSprites;

    VectorMapDescription(std::string identifier,
                         std::vector<std::shared_ptr<VectorMapSourceDescription>> vectorSources,
                         std::vector<std::shared_ptr<VectorLayerDescription>> layers,
                         std::optional<std::string> spriteBaseUrl,
                         std::map<std::string, std::shared_ptr<GeoJSONVTInterface>> geoJsonSources,
                         bool persistingSymbolPlacement,
                         std::optional<bool> use3xSprites):
    identifier(identifier), vectorSources(vectorSources), layers(layers), spriteBaseUrl(spriteBaseUrl),
    geoJsonSources(geoJsonSources), persistingSymbolPlacement(persistingSymbolPlacement), use3xSprites(use3xSprites) {}
};
