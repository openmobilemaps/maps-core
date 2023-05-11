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

#include "Tiled2dMapTileInfo.h"
#include "SymbolVectorLayerDescription.h"
#include "Value.h"
#include "SymbolInfo.h"
#include "StretchShaderInterface.h"
#include "OBB2D.h"
#include "MapInterface.h"
#include "Tiled2dMapVectorFontProvider.h"
#include "Tiled2dMapVectorSymbolLabelObject.h"
#include "Actor.h"
#include "SpriteData.h"
#include "TextLayerObject.h" // TODO: remove usage of TextLayerObject (and File)

class Tiled2dMapVectorSymbolObject {
public:
    Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                 const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                 const Tiled2dMapTileInfo &tileInfo,
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

    struct SymbolObjectInstanceCounts { int icons, textCharacters, stretchedIcons; };

    const SymbolObjectInstanceCounts getInstanceCounts() const;

    void setupIconProperties(std::vector<float> &positions, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData);
    void updateIconProperties(std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, int &countOffset, const double zoomIdentifier, const double scaleFactor);

    void setupTextProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier);
    void updateTextProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor);

    void setupStretchIconProperties(std::vector<float> &positions, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData);
    void updateStretchIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor);

    // TODO: Provide collision computation interface. But handle pre-computation/caching in SymbolGroup
    void setCollisionAt(float zoom, bool isCollision);

    bool hasCollision(float zoom);

    std::shared_ptr<FontLoaderResult> getFont() {
        if (labelObject) {
            return labelObject->getFont();
        }
        return nullptr;
    }

private:

    const FeatureContext featureContext;
    std::shared_ptr<SymbolInfo> textInfo;
    int64_t symbolSortKey;

    bool isInteractable = false;

    OBB2D orientedBoundingBox = OBB2D(Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0)));

    bool collides = true;

    std::map<float, bool> collisionMap = {};

    std::vector<float> modelMatrix;
    std::vector<float> iconModelMatrix;

    const std::weak_ptr<MapInterface> mapInterface;

    const std::shared_ptr<SymbolVectorLayerDescription> description;

    const ::Coord coordinate;

    SymbolObjectInstanceCounts instanceCounts = {0,0,0};

    Vec2D spriteSize = Vec2D(0.0, 0.0);

    Vec2D stretchSpriteSize = Vec2D(0.0, 0.0);
    std::optional<SpriteDesc> stretchSpriteInfo;

    std::shared_ptr<Tiled2dMapVectorSymbolLabelObject> labelObject;
};
