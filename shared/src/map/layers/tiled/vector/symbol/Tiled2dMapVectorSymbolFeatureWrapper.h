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
#include "StretchShaderInterface.h"
#include "ColorShaderInterface.h"
#include "OBB2D.h"
#include "Line2dLayerObject.h"

#include "Logger.h"

#include <mutex>
#include <unordered_map>

//#define DRAW_TEXT_BOUNDING_BOXES
//#define DRAW_COLLIDED_TEXT_BOUNDING_BOXES

class Tiled2dMapVectorSymbolFeatureWrapper {
public:
    FeatureContext featureContext;
    std::shared_ptr<SymbolInfo> textInfo;
    std::shared_ptr<TextLayerObject> textObject;

    int64_t symbolSortKey;

    std::vector<float> modelMatrix;

    std::vector<float> iconModelMatrix;

    void setCollisionAt(float zoom, bool isCollision) {
        collisionMap[zoom] = isCollision;
    }

    bool hasCollision(float zoom) {
        // TODO: make better.
        float minDiff = std::numeric_limits<float>::max();
        float min = std::numeric_limits<float>::max();

        bool collides = true;

        for(auto& cm : collisionMap) {
            float d = abs(zoom - cm.first);

            if(d < minDiff && d < min) {
                min = d;
                collides = cm.second;
            }
        }

        return collides;
    }

    bool collides = true;

    bool isInstanceRenderable = false;

    std::shared_ptr<Quad2dInterface> symbolObject;
    std::shared_ptr<GraphicsObjectInterface> symbolGraphicsObject;
    std::shared_ptr<StretchShaderInterface> symbolShader;

    OBB2D orientedBoundingBox = OBB2D(Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0)));

#ifdef DRAW_TEXT_BOUNDING_BOXES
    std::shared_ptr<ColorShaderInterface> boundingBoxShader = nullptr;
    std::shared_ptr<Quad2dInterface> boundingBox = nullptr;
#endif

#ifdef DRAW_TEXT_LINES
    std::shared_ptr<Line2dLayerObject> lineObject = nullptr;
#endif

    Tiled2dMapVectorSymbolFeatureWrapper() { };

    Tiled2dMapVectorSymbolFeatureWrapper(const FeatureContext &featureContext, const std::shared_ptr<SymbolInfo> &textInfo, const std::shared_ptr<TextLayerObject> &textObject, const int64_t &symbolSortKey)
    : featureContext(featureContext), textInfo(textInfo), textObject(textObject), symbolSortKey(symbolSortKey), modelMatrix(16, 0), iconModelMatrix(16, 0)
    {
    };

    std::map<float, bool> collisionMap = {};
};

struct WrapperCompare {
    bool operator() (std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> a, std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> b) const {
        if (a->symbolSortKey == b->symbolSortKey) {
            return a < b;
        }
        return a->symbolSortKey < b->symbolSortKey;
    }
};
