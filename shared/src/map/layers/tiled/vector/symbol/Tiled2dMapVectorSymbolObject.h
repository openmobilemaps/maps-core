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

#include "Tiled2dMapVersionedTileInfo.h"
#include "SymbolVectorLayerDescription.h"
#include "Value.h"
#include "SymbolInfo.h"
#include "StretchShaderInterface.h"
#include "MapInterface.h"
#include "Tiled2dMapVectorFontProvider.h"
#include "Tiled2dMapVectorSymbolLabelObject.h"
#include "Actor.h"
#include "SpriteData.h"
#include "TextLayerObject.h" // TODO: remove usage of TextLayerObject (and File)
#include "Tiled2dMapVectorLayerConfig.h"
#include "CollisionGrid.h"
#include "Vec3D.h"
#include "SymbolAnimationCoordinatorMap.h"
#include "VectorModificationWrapper.h"
#include "SymbolVectorLayerDescription.h"

class Tiled2dMapVectorSymbolObject {
public:
    Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                 const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                 const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                 const Tiled2dMapVersionedTileInfo &tileInfo,
                                 const std::string &layerIdentifier,
                                 const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                 const std::shared_ptr<FeatureContext> featureContext,
                                 const std::vector<FormattedStringEntry> &text,
                                 const std::string &fullText,
                                 const ::Vec2D &coordinate,
                                 const std::optional<std::vector<Vec2D>> &lineCoordinates,
                                 const std::vector<std::string> &fontList,
                                 const Anchor &textAnchor,
                                 const std::optional<double> &angle,
                                 const TextJustify &textJustify,
                                 const TextSymbolPlacement &textSymbolPlacement,
                                 const bool hideIcon,
                                 std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap,
                                 const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                 const UsedKeysCollection &usedKeys,
                                 const size_t symbolTileIndex,
                                 const bool hasCustomTexture,
                                 const double dpFactor,
                                 const bool persistingSymbolPlacement,
                                 bool is3d,
                                 const Vec3D &tileOrigin,
                                 const uint16_t styleIndex);

    ~Tiled2dMapVectorSymbolObject();

    void placedInCache();

    void removeFromCache();

    struct SymbolObjectInstanceCounts {
        int32_t icons, textCharacters, stretchedIcons;
    };

    const SymbolObjectInstanceCounts getInstanceCounts() const;

    void setupIconProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData, const std::optional<RectI> customUv);

    void updateIconProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, VectorModificationWrapper<float> &offsets, VectorModificationWrapper<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now, const Vec2I viewPortSize, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData);

    void setupTextProperties(VectorModificationWrapper<float> &textureCoordinates, VectorModificationWrapper<uint16_t> &styleIndices, int &countOffset, const double zoomIdentifier);
    void updateTextProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, VectorModificationWrapper<float> &styles, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now, const Vec2I viewPortSize, const std::vector<float>& vpMatrix, const Vec3D& origin);

    void setupStretchIconProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData);
    void updateStretchIconProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, VectorModificationWrapper<float> &stretchInfos, VectorModificationWrapper<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now, const Vec2I viewPortSize, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData);

    std::shared_ptr<FontLoaderResult> getFont() {
        if (labelObject) {
            return labelObject->getFont();
        }
        return nullptr;
    }

    double symbolSortKey;
    const size_t symbolTileIndex;

    std::optional<CollisionRectD> getViewportAlignedBoundingBox(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag);

    std::optional<std::vector<CollisionCircleD>> getMapAlignedBoundingCircles(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag);

    bool getIsOpaque();

    void setHideFromCollision(bool hide);

    void collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<CollisionGrid> collisionGrid);

    std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> onClickConfirmed(const CircleD &clickHitCircle, double zoomIdentifier, CollisionUtil::CollisionEnvironment &collisionEnvironment);

    void setAlpha(float alpha);

    void updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription, const UsedKeysCollection &usedKeys);

    std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator;

    bool isCoordinateOwner = false;

    const std::shared_ptr<FeatureContext> featureContext;

    bool hasCustomTexture;

    size_t customTexturePage = 0;
    int customTextureOffset = 0;
    std::string stringIdentifier;

    Vec3D tileOrigin = Vec3D(0,0,0);

private:
    double lastZoomEvaluation = -1;
    void evaluateStyleProperties(const double zoomIdentifier);

    void writePosition(const double x, const double y, const size_t offset, VectorModificationWrapper<float> &buffer);

    ::Vec2D getRenderCoordinates(Anchor iconAnchor, double rotation, double iconWidth, double iconHeight);

    std::shared_ptr<Tiled2dMapVectorLayerConfig> layerConfig;

    std::shared_ptr<Tiled2dMapVectorSymbolLabelObject> labelObject;

    std::shared_ptr<SymbolInfo> textInfo;

    bool isInteractable = false;

    const std::weak_ptr<MapInterface> mapInterface;

    std::shared_ptr<SymbolVectorLayerDescription> description;

    const ::Vec2D coordinate;
    ::Vec2D renderCoordinate = Vec2D(0, 0);
    Vec2D initialRenderCoordinateVec = Vec2D(0, 0);

    SymbolObjectInstanceCounts instanceCounts = {0,0,0};

    Vec2D spriteSize = Vec2D(0.0, 0.0);

    Vec2D stretchSpriteSize = Vec2D(0.0, 0.0);
    std::optional<SpriteDesc> stretchSpriteInfo;

    RectD iconBoundingBoxViewportAligned;
    RectD stretchIconBoundingBoxViewportAligned;

    SymbolAlignment iconRotationAlignment = SymbolAlignment::AUTO;

    bool isStyleZoomDependant = true;
    bool isStyleStateDependant = true;

    // the following flags are all optional, if they are set to -1 it is not set
    double lastIconUpdateScaleFactor = -1;
    double lastIconUpdateRotation = -1;
    float lastIconUpdateAlpha = -1;

    double lastStretchIconUpdateScaleFactor = -1;
    double lastStretchIconUpdateRotation = -1;

    double lastTextUpdateScaleFactor = -1;
    double lastTextUpdateRotation = -1;

    EvaluatedResult<bool> textAllowOverlap = false;
    EvaluatedResult<bool> iconAllowOverlap = false;

    bool persistingSymbolPlacement = false;

    double dpFactor = 1.0;
    float alpha = 1.0;
    bool isIconOpaque = true;
    bool isStretchIconOpaque = true;

    EvaluatedResult<double> iconOpacity = 0.0;
    EvaluatedResult<double> iconRotate = 0.0;
    EvaluatedResult<double> iconSize = 0.0;
    EvaluatedResult<std::string> iconImage = std::string();

    std::string lastIconImage;
    std::vector<float> iconTextFitPadding;
    TextSymbolPlacement textSymbolPlacement;
    SymbolAlignment boundingBoxRotationAlignment = SymbolAlignment::AUTO;
    float iconPadding = 0;
    Anchor iconAnchor = Anchor::CENTER;
    
    EvaluatedResult<Vec2F> iconOffset = Vec2F(0.0, 0.0);
    IconTextFit iconTextFit = IconTextFit::NONE;

    std::optional<RectI> customIconUv;

    size_t contentHash = 0;

    double maxCollisionZoom = -1;

    bool isPlaced();

    size_t crossTileIdentifier;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
    
    bool is3d;
    int positionSize;

    int32_t systemIdentifier;

    const std::optional<double> angle;
};
