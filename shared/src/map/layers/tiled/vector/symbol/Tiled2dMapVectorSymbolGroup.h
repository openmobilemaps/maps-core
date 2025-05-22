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
#include "Tiled2dMapVersionedTileInfo.h"
#include "SymbolVectorLayerDescription.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorSymbolSubLayerPositioningWrapper.h"
#include "SymbolInfo.h"
#include "SpriteData.h"
#include "Actor.h"
#include "TextInstancedInterface.h"
#include "Quad2dStretchedInstancedInterface.h"
#include "PolygonGroup2dLayerObject.h"
#include "Tiled2dMapVectorLayerSymbolDelegateInterface.h"
#include "CollisionGrid.h"
#include "RenderObjectInterface.h"
#include "SymbolObjectCollisionWrapper.h"
#include "VectorModificationWrapper.h"

//#define DRAW_TEXT_BOUNDING_BOX
//#define DRAW_TEXT_BOUNDING_BOX_WITH_COLLISIONS

class Tiled2dMapVectorSourceSymbolDataManager;

class Tiled2dMapVectorSymbolGroup : public ActorObject, public std::enable_shared_from_this<Tiled2dMapVectorSymbolGroup> {
public:
    Tiled2dMapVectorSymbolGroup(uint32_t groupId,
                                const std::weak_ptr<MapInterface> &mapInterface,
                                const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                const Tiled2dMapVersionedTileInfo &tileInfo,
                                const std::string &layerIdentifier,
                                const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription,
                                const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                const bool persistingSymbolPlacement,
                                const bool useCustomCrossTileIdentifier);

    void initialize(std::weak_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> weakFeatures,
                    int32_t featuresBase,
                    int32_t featuresCount,
                    std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap,
                    const WeakActor<Tiled2dMapVectorSourceSymbolDataManager> &symbolManagerActor,
                    float alpha = 1.0);

    void update(const double zoomIdentifier, const double rotation, const double scaleFactor, long long now, const Vec2I viewPortSize, const std::vector<float>& vpMatrix, const Vec3D& origin);

    void setupObjects(const std::shared_ptr<SpriteData> &spriteData, const std::shared_ptr<TextureHolderInterface> &spriteTexture, const std::optional<WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> &symbolDataManager = std::nullopt);

    std::vector<SymbolObjectCollisionWrapper> getSymbolObjectsForCollision();

    std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> onClickConfirmed(const CircleD &clickHitCircle, double zoomIdentifier, CollisionUtil::CollisionEnvironment &collisionEnvironment);

    void setAlpha(float alpha);

    void placedInCache();

    void removeFromCache();

    void clear();

    void updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription);

    std::vector<std::shared_ptr< ::RenderObjectInterface>> getRenderObjects();
private:

    inline std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper>
    getPositioning(std::vector<::Vec2D>::const_iterator &iterator, const std::vector<::Vec2D> &collection, const double interpolationValue);

    inline std::shared_ptr<Tiled2dMapVectorSymbolObject> createSymbolObject(const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                            const std::string &layerIdentifier,
                                                                            const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                                            const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                            const std::shared_ptr<FeatureContext> &featureContext,
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
                                                                            const size_t symbolTileIndex,
                                                                            const bool hasCustomTexture);

public:
    uint32_t groupId;
private:
    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolObject>> symbolObjects;

    const std::weak_ptr<MapInterface> mapInterface;
    std::shared_ptr<Tiled2dMapVectorLayerConfig> layerConfig;
    const Tiled2dMapVersionedTileInfo tileInfo;
    const std::string layerIdentifier;
    std::shared_ptr<SymbolVectorLayerDescription> layerDescription;
    const WeakActor<Tiled2dMapVectorFontProvider> fontProvider;
    const bool persistingSymbolPlacement = false;
    const bool useCustomCrossTileIdentifier = false;

    std::shared_ptr<Quad2dInstancedInterface> iconInstancedObject;
    std::shared_ptr<Quad2dStretchedInstancedInterface> stretchedInstancedObject;
    std::vector<std::shared_ptr<TextInstancedInterface>> textInstancedObjects;
    std::shared_ptr<PolygonGroup2dLayerObject> boundingBoxLayerObject;

    std::shared_ptr<TextureHolderInterface> spriteTexture;
    std::shared_ptr<SpriteData> spriteData;

    struct CustomIconDescriptor {
        VectorModificationWrapper<float> iconPositions;
        VectorModificationWrapper<float> iconScales;
        VectorModificationWrapper<float> iconRotations;
        VectorModificationWrapper<float> iconAlphas;
        VectorModificationWrapper<float> iconOffsets;
        VectorModificationWrapper<float> iconTextureCoordinates;
        std::shared_ptr<TextureHolderInterface> texture;
        std::shared_ptr<Quad2dInstancedInterface> renderObject;
        std::unordered_map<std::string, ::RectI> featureIdentifiersUv;

        CustomIconDescriptor(std::shared_ptr<TextureHolderInterface> texture, std::shared_ptr<Quad2dInstancedInterface> renderObject, std::unordered_map<std::string, ::RectI> featureIdentifiersUv, bool is3d): texture(texture), renderObject(renderObject), featureIdentifiersUv(featureIdentifiersUv) {
            auto count = featureIdentifiersUv.size();
            int positionSize = is3d ? 3 : 2;

            iconAlphas.resize(count, 0.0);
            iconRotations.resize(count, 0.0);
            iconScales.resize(count * 2, 0.0);
            iconPositions.resize(count * positionSize, 0.0);
            iconTextureCoordinates.resize(count * 4, 0.0);
            if (is3d) {
                iconOffsets.resize(count * 2, 0.0);
            }
        }
    };

    std::vector<CustomIconDescriptor> customTextures;


    VectorModificationWrapper<float> iconPositions;
    VectorModificationWrapper<float> iconScales;
    VectorModificationWrapper<float> iconRotations;
    VectorModificationWrapper<float> iconAlphas;
    VectorModificationWrapper<float> iconOffsets;
    VectorModificationWrapper<float> iconTextureCoordinates;

    struct TextDescriptor {
        std::shared_ptr<FontLoaderResult> fontResult;
        VectorModificationWrapper<float> textPositions;
        VectorModificationWrapper<float> textReferencePositions;
        VectorModificationWrapper<float> textScales;
        VectorModificationWrapper<float> textRotations;
        VectorModificationWrapper<uint16_t> textStyleIndices;
        VectorModificationWrapper<float> textStyles;
        VectorModificationWrapper<float> textTextureCoordinates;

        TextDescriptor(int32_t textStyleCount, size_t textCharactersCount, std::shared_ptr<FontLoaderResult> fontResult, bool is3d)
                : fontResult(fontResult) {
            textStyles.resize(textStyleCount * 10, 0.0);
            textStyleIndices.resize(textCharactersCount, 0);
            textRotations.resize(textCharactersCount, 0.0);
            textScales.resize(textCharactersCount * 2, 0.0);
            textPositions.resize(textCharactersCount * 2, 0.0);
            if (is3d) {
                textReferencePositions.resize(textCharactersCount * 3, 0.0);
            }
            textTextureCoordinates.resize(textCharactersCount * 4, 0.0);
        }
    };
    std::vector<std::shared_ptr<TextDescriptor>> textDescriptors;

    VectorModificationWrapper<float> stretchedIconPositions;
    VectorModificationWrapper<float> stretchedIconScales;
    VectorModificationWrapper<float> stretchedIconRotations;
    VectorModificationWrapper<float> stretchedIconAlphas;
    VectorModificationWrapper<float> stretchedIconStretchInfos;
    VectorModificationWrapper<float> stretchedIconTextureCoordinates;

    float alpha = 1.0;
    double dpFactor = 1.0;

    bool anyInteractable = false;

    bool isInitialized = false;

    bool is3d;
    Vec3D tileOrigin;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
    const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate;

#ifdef DRAW_TEXT_BOUNDING_BOX
    TextSymbolPlacement textSymbolPlacement;
    SymbolAlignment labelRotationAlignment;
    std::optional<CircleD> lastClickHitCircle;
#endif

    UsedKeysCollection usedKeys;
};
