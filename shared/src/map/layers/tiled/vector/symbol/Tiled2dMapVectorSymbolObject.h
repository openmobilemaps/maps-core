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
#include "SymbolAnimationCoordinatorMap.h"

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
                                 const ::Coord &coordinate,
                                 const std::optional<std::vector<Coord>> &lineCoordinates,
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
                                 const bool persistingSymbolPlacement);

    ~Tiled2dMapVectorSymbolObject();

    void placedInCache();

    struct SymbolObjectInstanceCounts { int icons, textCharacters, stretchedIcons; };

    const SymbolObjectInstanceCounts getInstanceCounts() const;

    void setupIconProperties(std::vector<float> &positions, std::vector<float> &rotations, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData, const std::optional<RectI> customUv);

    void updateIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now);

    void setupTextProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier);
    void updateTextProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now);

    void setupStretchIconProperties(std::vector<float> &positions, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData);
    void updateStretchIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now);

    std::shared_ptr<FontLoaderResult> getFont() {
        if (labelObject) {
            return labelObject->getFont();
        }
        return nullptr;
    }

    double symbolSortKey;
    const size_t symbolTileIndex;

    std::optional<CollisionRectF> getViewportAlignedBoundingBox(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag);

    std::optional<std::vector<CollisionCircleF>> getMapAlignedBoundingCircles(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag);

    bool getIsOpaque();

    void setHideFromCollision(bool hide);

    void collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<CollisionGrid> collisionGrid);

    std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> onClickConfirmed(const CircleD &clickHitCircle);

    void setAlpha(float alpha);

    void updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription, const UsedKeysCollection &usedKeys);

    std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator;

    bool isCoordinateOwner = false;

    const std::shared_ptr<FeatureContext> featureContext;

    bool hasCustomTexture;

    size_t customTexturePage = 0;
    int customTextureOffset = 0;
    std::string stringIdentifier;

private:
    double lastZoomEvaluation = -1;
    void evaluateStyleProperties(const double zoomIdentifier);

    ::Coord getRenderCoordinates(Anchor iconAnchor, double rotation, double iconWidth, double iconHeight);

    std::shared_ptr<Tiled2dMapVectorLayerConfig> layerConfig;

    std::shared_ptr<Tiled2dMapVectorSymbolLabelObject> labelObject;

    std::shared_ptr<SymbolInfo> textInfo;

    bool isInteractable = false;

    const std::weak_ptr<MapInterface> mapInterface;

    std::shared_ptr<SymbolVectorLayerDescription> description;

    const ::Coord coordinate;
    ::Coord renderCoordinate = Coord(0, 0, 0, 0);
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

    double lastStretchIconUpdateScaleFactor;
    double lastStretchIconUpdateRotation;

    double lastTextUpdateScaleFactor;
    double lastTextUpdateRotation;

    bool textAllowOverlap;
    bool iconAllowOverlap;

    bool persistingSymbolPlacement = false;

    double dpFactor = 1.0;
    float alpha = 1.0;
    bool isIconOpaque = true;
    bool isStretchIconOpaque = true;

    float iconOpacity;
    float iconRotate;
    float iconSize;
    std::vector<float> iconTextFitPadding;
    TextSymbolPlacement textSymbolPlacement;
    SymbolAlignment boundingBoxRotationAlignment = SymbolAlignment::AUTO;
    float iconPadding = 0;
    Anchor iconAnchor;
    Vec2F iconOffset = Vec2F(0.0, 0.0);
    IconTextFit iconTextFit = IconTextFit::NONE;

    size_t contentHash = 0;

    double maxCollisionZoom = -1;

    bool isPlaced();

    size_t crossTileIdentifier;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
};
