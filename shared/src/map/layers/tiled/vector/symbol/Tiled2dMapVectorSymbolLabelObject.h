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

#include "Vec2F.h"
#include "RectD.h"
#include "CircleD.h"
#include "CollisionPrimitives.h"
#include "SymbolVectorLayerDescription.h"
#include "Value.h"
#include "SymbolInfo.h"
#include "StretchShaderInterface.h"
#include "MapInterface.h"
#include "Tiled2dMapVectorFontProvider.h"
#include "CoordinateConversionHelperInterface.h"
#include "Actor.h"
#include "BoundingBox.h"
#include "SpriteData.h"
#include "SymbolAnimationCoordinator.h"
#include "Vec2DHelper.h"

class SymbolAnimationCoordinator;

class Tiled2dMapVectorSymbolLabelObject {
public:
    Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                      const std::shared_ptr<FeatureContext> featureContext,
                                      const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                      const std::vector<FormattedStringEntry> &text,
                                      const std::string &fullText,
                                      const ::Coord &coordinate,
                                      const std::optional<std::vector<Coord>> &lineCoordinates,
                                      const Anchor &textAnchor,
                                      const std::optional<double> &angle,
                                      const TextJustify &textJustify,
                                      const std::shared_ptr<FontLoaderResult> fontResult,
                                      const Vec2F &offset,
                                      const double radialOffset,
                                      const double lineHeight,
                                      const double letterSpacing,
                                      const int64_t maxCharacterWidth,
                                      const double maxCharacterAngle,
                                      const SymbolAlignment rotationAlignment,
                                      const TextSymbolPlacement &textSymbolPlacement,
                                      std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator,
                                      const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                      double dpFactor);

    int getCharacterCount();

    void setupProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier);

    void updateProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation, const float alpha, const bool isCoordinateOwner, long long now);

    std::shared_ptr<FontLoaderResult> getFont() {
        return fontResult;
    }

    void updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription);

    std::optional<CollisionRectD> boundingBoxViewportAligned = std::nullopt;
    std::optional<std::vector<CircleD>> boundingBoxCircles = std::nullopt;

    bool isOpaque = true;

    Vec2D dimensions = Vec2D(0.0, 0.0);
private:

    void updatePropertiesPoint(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation);
    double updatePropertiesLine(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation);

    bool isStyleStateDependant = true;
    double lastZoomEvaluation = -1;
    void evaluateStyleProperties(const double zoomIdentifier);

    std::pair<int, double> findReferencePointIndices();
    
    inline Vec2D pointAtIndex(const std::pair<int, double> &index, bool useRender) {
        const auto &s = useRender ? renderLineCoordinates[index.first] : (*lineCoordinates)[index.first];
        const auto &e = useRender ?  renderLineCoordinates[index.first + 1 < renderLineCoordinatesCount ? (index.first + 1) : index.first] : (*lineCoordinates)[index.first + 1 < renderLineCoordinatesCount ? (index.first + 1) : index.first];
        return Vec2D(s.x + (e.x - s.x) * index.second, s.y + (e.y - s.y) * index.second);
    }

    inline std::pair<int, double> indexAtDistance(const std::pair<int, double> &index, double distance, const std::optional<Vec2D> &indexCoord) {
        auto current = indexCoord ? *indexCoord : pointAtIndex(index, true);
        auto currentIndex = index;
        auto dist = std::abs(distance);

        if(distance >= 0) {
            auto start = std::min(index.first + 1, (int)renderLineCoordinatesCount - 1);

            for(int i = start; i < renderLineCoordinatesCount; i++) {
                const auto &next = renderLineCoordinates.at(i);

                const double d = Vec2DHelper::distance(current, Vec2D(next.x, next.y));

                if(dist > d) {
                    dist -= d;
                    current.x = next.x;
                    current.y = next.y;
                    currentIndex = std::make_pair(i, 0.0);
                } else {
                    return std::make_pair(currentIndex.first, currentIndex.second + dist / d * (1.0 - currentIndex.second));
                }
            }
        } else {
            auto start = index.first;

            for(int i = start; i >= 0; i--) {
                const auto &next = renderLineCoordinates.at(i);

                const auto d = Vec2DHelper::distance(current, Vec2D(next.x, next.y));

                if(dist > d) {
                    dist -= d;
                    current.x = next.x;
                    current.y = next.y;
                    currentIndex = std::make_pair(i, 0.0);
                } else {
                    if(i == currentIndex.first) {
                        return std::make_pair(i, currentIndex.second - currentIndex.second * dist / d);
                    } else {
                        return std::make_pair(i, 1.0 - dist / d);
                    }
                }
            }

        }

        return currentIndex;
    }

    std::shared_ptr<SymbolVectorLayerDescription> description;
    const std::shared_ptr<FeatureContext> featureContext;
    const TextSymbolPlacement textSymbolPlacement;
    const SymbolAlignment rotationAlignment;
    TextJustify textJustify;
    const Anchor textAnchor;
    const Vec2F offset;
    const double radialOffset;

    float spaceAdvance = 0.0f;

    const double lineHeight;
    const double letterSpacing;
    const double maxCharacterAngle;

    const std::shared_ptr<FontLoaderResult> fontResult;

    Coord referencePoint = Coord(0,0,0,0);
    float referenceSize;


    std::vector<Vec2D> centerPositions;

    struct SplitInfo {
        SplitInfo(int g, float s) : glyphIndex(g), scale(s) {};
        int glyphIndex;
        float scale;
    };

    int characterCount = 0;
    std::vector<SplitInfo> splittedTextInfo;
    int numSymbols;
    int spaceIndex = -1;

    const std::string fullText;

    size_t renderLineCoordinatesCount;
    std::vector<Coord> renderLineCoordinates;
    std::optional<std::vector<Coord>> lineCoordinates;

    double textSize = 0;
    double textRotate = 0;
    double textPadding = 0;
    SymbolAlignment textAlignment = SymbolAlignment::AUTO;

    double textOpacity;
    Color textColor = Color(0.0 ,0.0, 0.0, 0.0);
    Color haloColor = Color(0.0 ,0.0, 0.0, 0.0);
    double haloWidth;
    double haloBlur;

    std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator;
    static constexpr double collisionDistanceBias = 0.75;

    bool wasReversed = false;

    const std::shared_ptr<Tiled2dMapVectorStateManager> stateManager;

    double dpFactor = 1.0;
};
