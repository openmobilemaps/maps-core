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

#include "Tiled2dMapVectorSymbolObject.h"
#include "MapInterface.h"
#include "Tiled2dMapTileInfo.h"
#include "SymbolVectorLayerDescription.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorSymbolSubLayerPositioningWrapper.h"
#include "SymbolInfo.h"
#include "SpriteData.h"
#include "Actor.h"
#include "TextInstancedInterface.h"
#include "Quad2dStretchedInstancedInterface.h"
#include "PolygonGroup2dLayerObject.h"

//#define DRAW_TEXT_BOUNDING_BOX

class Tiled2dMapVectorSymbolGroup : public ActorObject {
public:
    Tiled2dMapVectorSymbolGroup(const std::weak_ptr<MapInterface> &mapInterface,
                                const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                const Tiled2dMapTileInfo &tileInfo,
                                const std::string &layerIdentifier,
                                const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription,
                                const std::shared_ptr<SpriteData> &spriteData,
                                const std::shared_ptr<TextureHolderInterface> &spriteTexture);

    bool initialize(const std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features);


    void update(const double zoomIdentifier, const double scaleFactor);

    void setupObjects();
    
    std::shared_ptr<Quad2dInstancedInterface> iconInstancedObject;
    std::shared_ptr<Quad2dStretchedInstancedInterface> stretchedInstancedObject;
    std::shared_ptr<TextInstancedInterface> textInstancedObject;

    void collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<std::vector<OBB2D>> placements);

    std::shared_ptr<PolygonGroup2dLayerObject> boundingBoxLayerObject;
private:

    inline std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection);

    inline std::shared_ptr<Tiled2dMapVectorSymbolObject> createSymbolObject(const Tiled2dMapTileInfo &tileInfo,
                                                                     const std::string &layerIdentifier,
                                                                     const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                                     const FeatureContext &featureContext,
                                                                     const std::vector<FormattedStringEntry> &text,
                                                                     const std::string &fullText,
                                                                     const ::Coord &coordinate,
                                                                     const std::optional<std::vector<Coord>> &lineCoordinates,
                                                                     const std::vector<std::string> &fontList,
                                                                     const Anchor &textAnchor,
                                                                     const std::optional<double> &angle,
                                                                     const TextJustify &textJustify,
                                                                     const TextSymbolPlacement &textSymbolPlacement);

private:
    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolObject>> symbolObjects;



    const std::weak_ptr<MapInterface> mapInterface;
    const Tiled2dMapTileInfo tileInfo;
    const std::string layerIdentifier;
    std::shared_ptr<SymbolVectorLayerDescription> layerDescription;
    const WeakActor<Tiled2dMapVectorFontProvider> fontProvider;

    std::shared_ptr<TextureHolderInterface> spriteTexture;
    std::shared_ptr<SpriteData> spriteData;

    std::vector<float> iconPositions;
    std::vector<float> iconScales;
    std::vector<float> iconRotations;
    std::vector<float> iconAlphas;
    std::vector<float> iconTextureCoordinates;

    std::shared_ptr<FontLoaderResult> fontResult;
    std::vector<float> textPositions;
    std::vector<float> textScales;
    std::vector<float> textRotations;
    std::vector<uint16_t> textStyleIndices;
    std::vector<float> textStyles;
    std::vector<float> textTextureCoordinates;

    std::vector<float> stretchedIconPositions;
    std::vector<float> stretchedIconScales;
    std::vector<float> stretchedIconRotations;
    std::vector<float> stretchedIconAlphas;
    std::vector<float> stretchedIconStretchInfos;
    std::vector<float> stretchedIconTextureCoordinates;
};
