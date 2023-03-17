/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Value.h"
#include "SymbolInfo.h"
#include "TextLayerObject.h"
#include "Quad2dInterface.h"
#include "GraphicsObjectInterface.h"
#include "AlphaShaderInterface.h"
#include "OBB2D.h"
#include "Line2dLayerObject.h"

struct Tiled2dMapVectorSymbolFeatureWrapper {
    FeatureContext featureContext;
    std::shared_ptr<SymbolInfo> textInfo;
    std::shared_ptr<TextLayerObject> textObject;

    int64_t symbolSortKey;

    std::vector<float> modelMatrix;

    std::vector<float> iconModelMatrix;

    bool collides = true;

    std::shared_ptr<Quad2dInterface> symbolObject;
    std::shared_ptr<GraphicsObjectInterface> symbolGraphicsObject;
    std::shared_ptr<AlphaShaderInterface> symbolShader;

    OBB2D orientedBoundingBox = OBB2D(Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0)));

    std::optional<Quad2dD> projectedTextQuad = std::nullopt;

#ifdef DRAW_TEXT_BOUNDING_BOXES
    std::shared_ptr<Quad2dInterface> boundingBox = nullptr;
#endif

#ifdef DRAW_TEXT_LINES
    std::shared_ptr<Line2dLayerObject> lineObject = nullptr;
#endif

    Tiled2dMapVectorSymbolFeatureWrapper() {};

    Tiled2dMapVectorSymbolFeatureWrapper(const FeatureContext &featureContext, const std::shared_ptr<SymbolInfo> &textInfo, const std::shared_ptr<TextLayerObject> &textObject, const int64_t symbolSortKey) : featureContext(featureContext), textInfo(textInfo), textObject(textObject), symbolSortKey(symbolSortKey),  modelMatrix(16, 0), iconModelMatrix(16, 0) { };
};

struct WrapperCompare {
    bool operator() (std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> a, std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> b) const {
        if (a->symbolSortKey == b->symbolSortKey) {
            return a < b;
        }
        return a->symbolSortKey < b->symbolSortKey;
    }
};
