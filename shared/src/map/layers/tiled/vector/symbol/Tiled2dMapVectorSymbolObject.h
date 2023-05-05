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
#include "Actor.h"
#include "TextLayerObject.h" // TODO: remove usage of TextLayerObject (and File)

class Tiled2dMapVectorSymbolObject {
public:
    Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                 const Actor<Tiled2dMapVectorFontProvider> &fontProvider,
                                 const Tiled2dMapTileInfo &tileInfo,
                                 const std::string &layerIdentifier,
                                 const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                 const std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>> &symbolInfo);

    // TODO: Provide collision computation interface. But handle pre-computation/caching in SymbolGroup
    void setCollisionAt(float zoom, bool isCollision);

    bool hasCollision(float zoom);

    FeatureContext featureContext;
    std::shared_ptr<SymbolInfo> textInfo;
    int64_t symbolSortKey;

    bool isInteractable = false;

    // TODO: Move to instanced objects in SymbolGroup
    std::shared_ptr<TextLayerObject> textObject;
    std::shared_ptr<Quad2dInterface> symbolObject;
    std::shared_ptr<GraphicsObjectInterface> symbolGraphicsObject;
    std::shared_ptr<StretchShaderInterface> symbolShader;

    OBB2D orientedBoundingBox = OBB2D(Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0)));
    bool collides = true;
    std::map<float, bool> collisionMap = {};

    std::vector<float> modelMatrix;
    std::vector<float> iconModelMatrix;


#ifdef DRAW_TEXT_BOUNDING_BOXES
    std::shared_ptr<ColorShaderInterface> boundingBoxShader = nullptr;
    std::shared_ptr<Quad2dInterface> boundingBox = nullptr;
#endif

#ifdef DRAW_TEXT_LINES
    std::shared_ptr<Line2dLayerObject> lineObject = nullptr;
#endif

    struct ObjectCompare {
        bool operator() (std::shared_ptr<Tiled2dMapVectorSymbolObject> a, std::shared_ptr<Tiled2dMapVectorSymbolObject> b) const {
            if (a->symbolSortKey == b->symbolSortKey) {
                return a < b;
            }
            return a->symbolSortKey < b->symbolSortKey;
        }
    };

};
