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

#include "SymbolLineGeometryCache.h"
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

class Tiled2dMapVectorSymbolLabelObject {
public:
    Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                      const std::shared_ptr<FeatureContext> featureContext,
                                      const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                      const std::vector<FormattedStringEntry> &text,
                                      const std::string &fullText,
                                      const ::Vec2D &coordinate,
                                      const std::shared_ptr<SymbolLineGeometryCache> &lineGeometryCache,
                                      const Anchor &textAnchor,
                                      const TextJustify &textJustify,
                                      const std::shared_ptr<FontLoaderResult> fontResult,
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

    void invalidateLayoutCaches();

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

    struct PointLabelLayoutCache {
        float angle = 0;
        float fontSize = 0;
        Vec2D textOffset = Vec2D(0.0, 0.0);
        float textPadding = 0;
        Vec2I viewportSize = Vec2I(0, 0);
        bool valid = false;

        bool matches(float angle_, float fontSize_, const Vec2D &textOffset_, float textPadding_,
                     const Vec2I &viewportSize_, bool is3d) const {
            return valid && angle == angle_ && fontSize == fontSize_ && textOffset == textOffset_ &&
                   textPadding == textPadding_ && (!is3d || viewportSize == viewportSize_);
        }
        void invalidate() { valid = false; }
    };

    PointLabelLayoutCache pointLabelLayoutCache;

    struct LineLabelLayoutCache {
        float fontSize = 0;
        float rotation = 0;
        bool wasReversed = false;
        float appliedTextOffsetY = 0;
        float scaledTextPadding = 0;
        bool valid = false;

        bool matches(float fontSize_, bool wasReversed_, float rotation_, float appliedTextOffsetY_,
                     float scaledTextPadding_) const {
            return valid && fontSize == fontSize_ && wasReversed == wasReversed_ && rotation == rotation_ &&
                   appliedTextOffsetY == appliedTextOffsetY_ && scaledTextPadding == scaledTextPadding_;
        }
        void invalidate() { valid = false; }
    };

    LineLabelLayoutCache lineLabelLayoutCache;

    bool isStyleStateDependant = true;
    double lastZoomEvaluation = -1;
    void evaluateStyleProperties(const double zoomIdentifier);

    LineSegmentIndex findReferencePointIndices() {
        const auto &point = is3d ? referencePointScreen : referencePoint;
        return lineGeometryCache->findReferencePoint(point, wasReversed);
    }

    inline Vec2D pointAtIndex(const LineSegmentIndex &index, bool useRender) {
        return useRender ? lineGeometryCache->renderPointAt(index.index, index.percentage, wasReversed)
                         : lineGeometryCache->tilePointAt(index.index, index.percentage, wasReversed);
    }

    inline Vec2D screenPointAtIndex(const LineSegmentIndex &index) {
        return lineGeometryCache->screenPointAt(index.index, index.percentage, wasReversed);
    }

    Vec2D pointForIndex(const LineSegmentIndex &index, const std::optional<Vec2D> &indexCoord) {
        return is3d ? screenPointAtIndex(index) : (indexCoord ? *indexCoord : pointAtIndex(index, true));
    }

    inline void indexAtDistance(const LineSegmentIndex &index, const Vec2D &currentPoint, double distance, LineSegmentIndex &result) {
        lineGeometryCache->indexAtDistance(index, currentPoint, distance, wasReversed, result);
    }

    std::shared_ptr<SymbolVectorLayerDescription> description;
    const std::shared_ptr<FeatureContext> featureContext;
    const TextSymbolPlacement textSymbolPlacement;
    const SymbolAlignment rotationAlignment;
    TextJustify textJustify;
    const Anchor textAnchor;
    const double radialOffset;

    float spaceAdvance = 0.0f;

    const double lineHeight;
    const double letterSpacing;
    const double maxCharacterAngle;

    const std::shared_ptr<FontLoaderResult> fontResult;

    Vec3D referencePoint = Vec3D(0.0, 0.0, 0.0);
    Vec3D cartesianReferencePoint = Vec3D(0.0, 0.0, 0.0);
    Vec3D referencePointScreen = Vec3D(0.0, 0.0, 0);

    std::shared_ptr<SymbolLineGeometryCache> lineGeometryCache;
    LineSegmentIndex currentReferencePointIndex;

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

    std::vector<Vec2D> centerPositions;
    std::vector<size_t> lineEndIndices;

    FeatureValueEvaluationResult<double> textSize = 0.0;
    FeatureValueEvaluationResult<Vec2F> textOffset = Vec2F(0.0, 0.0);
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
