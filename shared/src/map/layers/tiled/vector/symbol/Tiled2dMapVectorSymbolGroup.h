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
                                const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate);

    void initialize(std::weak_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> weakFeatures,
                    int32_t featuresBase,
                    int32_t featuresCount,
                    std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap,
                    const WeakActor<Tiled2dMapVectorSourceSymbolDataManager> &symbolManagerActor,
                    float alpha = 1.0);

    void update(const double zoomIdentifier, const double rotation, const double scaleFactor, long long now);

    void setupObjects(const std::shared_ptr<SpriteData> &spriteData, const std::shared_ptr<TextureHolderInterface> &spriteTexture, const std::optional<WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> &symbolDataManager = std::nullopt);

    std::vector<SymbolObjectCollisionWrapper> getSymbolObjectsForCollision();

    std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> onClickConfirmed(const CircleD &clickHitCircle);

    void setAlpha(float alpha);

    void placedInCache();

    void clear();

    void updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription);

    std::vector<std::shared_ptr< ::RenderObjectInterface>> getRenderObjects();
private:

    inline std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper>
    getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> &collection, const double interpolationValue);

    inline std::shared_ptr<Tiled2dMapVectorSymbolObject> createSymbolObject(const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                            const std::string &layerIdentifier,
                                                                            const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                                            const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                            const std::shared_ptr<FeatureContext> &featureContext,
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

    std::shared_ptr<Quad2dInstancedInterface> iconInstancedObject;
    std::shared_ptr<Quad2dStretchedInstancedInterface> stretchedInstancedObject;
    std::shared_ptr<TextInstancedInterface> textInstancedObject;
    std::shared_ptr<PolygonGroup2dLayerObject> boundingBoxLayerObject;

    std::shared_ptr<TextureHolderInterface> spriteTexture;
    std::shared_ptr<SpriteData> spriteData;

    struct CustomIconDescriptor {
        std::vector<float> iconPositions;
        std::vector<float> iconScales;
        std::vector<float> iconRotations;
        std::vector<float> iconAlphas;
        std::vector<float> iconTextureCoordinates;
        std::shared_ptr<TextureHolderInterface> texture;
        std::shared_ptr<Quad2dInstancedInterface> renderObject;
        std::unordered_map<std::string, ::RectI> featureIdentifiersUv;

        CustomIconDescriptor(std::shared_ptr<TextureHolderInterface> texture, std::shared_ptr<Quad2dInstancedInterface> renderObject, std::unordered_map<std::string, ::RectI> featureIdentifiersUv): texture(texture), renderObject(renderObject), featureIdentifiersUv(featureIdentifiersUv) {
            auto count = featureIdentifiersUv.size();

            iconAlphas.resize(count, 0.0);
            iconRotations.resize(count, 0.0);
            iconScales.resize(count * 2, 0.0);
            iconPositions.resize(count * 2, 0.0);
            iconTextureCoordinates.resize(count * 4, 0.0);
        }
    };

    std::vector<CustomIconDescriptor> customTextures;


    std::vector<float> iconPositions;
    std::vector<float> iconScales;
    std::vector<float> iconRotations;
    std::vector<float> iconAlphas;
    std::vector<float> iconTextureCoordinates;

    std::shared_ptr<FontLoaderResult> fontResult;
    std::vector<float> textPositions;
    std::vector<float> textScales;
    std::vector<float> textRotations;
    std::vector<uint16_t> textStyleIndices;
    std::vector<float> textStyles;
    std::vector<float> textTextureCoordinates;

    std::vector<float> stretchedIconPositions;
    std::vector<float> stretchedIconScales;
    std::vector<float> stretchedIconRotations;
    std::vector<float> stretchedIconAlphas;
    std::vector<float> stretchedIconStretchInfos;
    std::vector<float> stretchedIconTextureCoordinates;

    float alpha = 1.0;
    double dpFactor = 1.0;

    bool anyInteractable = false;

    bool isInitialized = false;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
    const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate;

#ifdef DRAW_TEXT_BOUNDING_BOX
    TextSymbolPlacement textSymbolPlacement;
    SymbolAlignment labelRotationAlignment;
#endif

    UsedKeysCollection usedKeys;
};
