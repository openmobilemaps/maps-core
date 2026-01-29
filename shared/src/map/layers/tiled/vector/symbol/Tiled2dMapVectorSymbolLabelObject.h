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
#include "MapCameraInterface.h"
#include "VectorModificationWrapper.h"

class SymbolAnimationCoordinator;

struct DistanceIndex {
    int index;
    double percentage;

    DistanceIndex(int index_, double percentage_)
    : index(std::move(index_))
    , percentage(std::move(percentage_))
    {}
};

class Tiled2dMapVectorSymbolLabelObject {
public:
    Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                      const std::shared_ptr<FeatureContext> featureContext,
                                      const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                      const std::vector<FormattedStringEntry> &text,
                                      const std::string &fullText,
                                      const ::Vec2D &coordinate,
                                      const std::optional<std::vector<Vec2D>> &lineCoordinates,
                                      const Anchor &textAnchor,
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
                                      double dpFactor,
                                      bool is3d,
                                      const Vec3D &tileOrigin,
                                      const uint16_t styleIndex,
                                      const int32_t systemIdentifier);

    int getCharacterCount();

    void setupProperties(VectorModificationWrapper<float> &textureCoordinates, VectorModificationWrapper<uint16_t> &styleIndices, int &countOffset, const double zoomIdentifier);

    void updateProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, VectorModificationWrapper<float> &styles, int &countOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation, const float alpha, const bool isCoordinateOwner, int64_t now, const Vec2I &viewportSize, const std::vector<float>& vpMatrix, const Vec3D& origin);

    std::shared_ptr<FontLoaderResult> getFont() {
        return fontResult;
    }

    void updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription);

    std::optional<CollisionRectD> boundingBoxViewportAligned = std::nullopt;
    std::optional<std::vector<CircleD>> boundingBoxCircles = std::nullopt;

    bool isOpaque = true;
    bool wasReversed = false;

    bool isPlaced = true;

    Vec2D dimensions = Vec2D(0.0, 0.0);
    Vec3D tileOrigin = Vec3D(0,0,0);

private:
    void precomputeMedianIfNeeded();

    void setupCameraFor3D(const std::vector<float>& vpMatrix, const Vec3D& origin, const Vec2I& viewportSize);

    void writePosition(const double x, const double y, const size_t offset, VectorModificationWrapper<float> &buffer);

    void updatePropertiesPoint(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, int &countOffset, float alphaFactor, const double zoomIdentifier, const double scaleFactor, const double rotation, const Vec2I &viewportSize);
    double updatePropertiesLine(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, int &countOffset, float alphaFactor, const double zoomIdentifier, const double scaleFactor, const double rotation, const Vec2I &viewportSize);

    bool isStyleStateDependant = true;
    double lastZoomEvaluation = -1;
    void evaluateStyleProperties(const double zoomIdentifier);

    DistanceIndex findReferencePointIndices();

    inline Vec2D pointAtIndex(const DistanceIndex &index, bool useRender) {
        const auto &s = useRender ? renderLineCoordinates[index.index] : Vec2DHelper::toVec3D((*lineCoordinates)[index.index]);
        const auto &e = useRender ?  renderLineCoordinates[index.index + 1 < renderLineCoordinatesCount ? (index.index + 1) : index.index] : Vec2DHelper::toVec3D((*lineCoordinates)[index.index + 1 < renderLineCoordinatesCount ? (index.index + 1) : index.index]);
        return Vec2D(s.x + (e.x - s.x) * index.percentage,
                     s.y + (e.y - s.y) * index.percentage);
    }

    inline Vec2D screenPointAtIndex(const DistanceIndex &index) {
        const auto &s = screenLineCoordinates[index.index];
        const auto &e = screenLineCoordinates[index.index + 1 < renderLineCoordinatesCount ? (index.index + 1) : index.index];
        return Vec2D(s.x + (e.x - s.x) * index.percentage, s.y + (e.y - s.y) * index.percentage);
    }

    Vec2D pointForIndex(const DistanceIndex &index, const std::optional<Vec2D> &indexCoord) {
        return is3d ? screenPointAtIndex(index) : (indexCoord ? *indexCoord : pointAtIndex(index, true));
    }

    inline void indexAtDistance(const DistanceIndex &index, const Vec2D& currentPoint, double distance, DistanceIndex& result) {
        auto dist = std::abs(distance);
        auto current = currentPoint;

        int currentI = index.index;
        double currentPercentage = index.percentage;

        if(distance >= 0) {
            auto start = std::min(index.index + 1, (int)renderLineCoordinatesCount - 1);

            for(int i = start; i < renderLineCoordinatesCount; i++) {
                const auto &next = screenLineCoordinates[i];

                const double d = Vec2DHelper::distance(current, Vec2D(next.x, next.y));

                if(dist > d) {
                    dist -= d;
                    current.x = next.x;
                    current.y = next.y;
                    currentI = i;
                    currentPercentage = 0;
                } else {
                    result.index = currentI;
                    result.percentage = currentPercentage + dist / d * (1.0 - currentPercentage);
                    return;
                }
            }
        } else {
            auto start = index.index;

            for(int i = start; i >= 0; i--) {
                const auto &next = screenLineCoordinates[i];

                const auto d = Vec2DHelper::distance(current, Vec2D(next.x, next.y));

                if(dist > d) {
                    dist -= d;
                    current.x = next.x;
                    current.y = next.y;

                    currentI = i;
                    currentPercentage = 0.0;
                } else {
                    if(i == currentI) {
                        result.index = i;
                        result.percentage = currentPercentage - currentPercentage * dist / d;
                        return;
                    } else {
                        result.index = i;
                        result.percentage = 1.0 - dist / d;
                        return;
                    }
                }
            }
        }

        result = DistanceIndex(currentI, currentPercentage);
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

    Vec3D referencePoint = Vec3D(0.0, 0.0, 0.0);
    Vec3D cartesianReferencePoint = Vec3D(0.0, 0.0, 0.0);
    Vec3D referencePointScreen = Vec3D(0.0, 0.0, 0);


    std::vector<Vec2D> centerPositions;
    std::vector<size_t> lineEndIndices;

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
    std::vector<Vec3D> renderLineCoordinates;
    std::vector<Vec3D> screenLineCoordinates;
    std::vector<Vec3D> cartesianRenderLineCoordinates;
    std::optional<std::vector<Vec2D>> lineCoordinates;
    DistanceIndex currentReferencePointIndex = DistanceIndex(0, 0.0);

    FeatureValueEvaluationResult<double> textSize = 0.0;
    FeatureValueEvaluationResult<double> textRotate = 0.0;
    FeatureValueEvaluationResult<double> textPadding = 0.0;
    FeatureValueEvaluationResult<SymbolAlignment> textAlignment = SymbolAlignment::AUTO;

    FeatureValueEvaluationResult<double> textOpacity = 0.0;
    FeatureValueEvaluationResult<Color> textColor = Color(0.0 ,0.0, 0.0, 0.0);
    FeatureValueEvaluationResult<Color> haloColor = Color(0.0 ,0.0, 0.0, 0.0);
    FeatureValueEvaluationResult<double> haloWidth = 0.0;
    FeatureValueEvaluationResult<double> haloBlur = 0.0;

    std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator;
    static constexpr double collisionDistanceBias = 0.75;

    const std::shared_ptr<Tiled2dMapVectorStateManager> stateManager;

    double dpFactor = 1.0;
    bool is3d;
    int positionSize;

    double medianLastBaseLine = 0.0;

    uint16_t styleIndex = 0;
};
