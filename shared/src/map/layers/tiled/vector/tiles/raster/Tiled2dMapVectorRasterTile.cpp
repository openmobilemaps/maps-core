/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorRasterTile.h"
#include "MapCameraInterface.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "RasterShaderInterface.h"
#include "RenderPass.h"
#include "Tiled2dMapVectorStyleParser.h"
#include "TessellationSettings.h"
#include "Logger.h"
#include <algorithm>

Tiled2dMapVectorRasterTile::Tiled2dMapVectorRasterTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                       const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                                                       const Tiled2dMapVersionedTileInfo &tileInfo,
                                                       const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                       const std::shared_ptr<RasterVectorLayerDescription> &description,
                                                       const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                       const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
                                                       : Tiled2dMapVectorTile(mapInterface, vectorLayer, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager),
                                                       usedKeys(description->getUsedKeys()), zoomInfo(layerConfig->getZoomInfo()) {
    isStyleZoomDependant = description->textureLookup.has_value() || usedKeys.containsUsedKey(ValueKeys::ZOOM);
    isStyleStateDependant = usedKeys.isStateDependant();
    auto pMapInterface = mapInterface.lock();
    if (pMapInterface) {
        
    #if HARDWARE_TESSELLATION_SUPPORTED
        auto shader = pMapInterface->is3d() ? pMapInterface->getShaderFactory()->createQuadTessellatedShader() : pMapInterface->getShaderFactory()->createRasterShader();
        auto quad = pMapInterface->is3d() ? pMapInterface->getGraphicsObjectFactory()->createQuadTessellated(shader->asShaderProgramInterface()) :
            pMapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
    #else
        auto shader = pMapInterface->is3d() ? pMapInterface->getShaderFactory()->createUnitSphereRasterShader() : pMapInterface->getShaderFactory()->createRasterShader();
        auto quad = pMapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
    #endif
        
        shader->asShaderProgramInterface()->setBlendMode(description->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        
    #ifdef DEBUG
        quad->asGraphicsObject()->setDebugLabel(description->identifier + "_" + tileInfo.tileInfo.to_string_short());
    #endif
        tileObject = std::make_shared<Textured2dLayerObject>(quad, shader, pMapInterface, pMapInterface->is3d());
        tileObject->setRectCoord(tileInfo.tileInfo.bounds, RASTER_TILE_RENDER_OVERLAP_FACTOR);

        if  (pMapInterface->is3d()) {
            quad->setSubdivisionFactor(std::clamp(subdivisionFactor + tileInfo.tileInfo.tessellationFactor, 0, 5));
        }
    }
}

void Tiled2dMapVectorRasterTile::updateRasterLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                        const Tiled2dMapVectorTileDataRaster &tileData) {
    Tiled2dMapVectorTile::updateRasterLayerDescription(description, tileData);
    auto rasterDescription = std::static_pointer_cast<RasterVectorLayerDescription>(description);
    isStyleZoomDependant = rasterDescription->textureLookup.has_value() || usedKeys.containsUsedKey(ValueKeys::ZOOM);
    isStyleStateDependant = usedKeys.isStateDependant();
    lastZoom = std::nullopt;
    setRasterTileData(tileData);
}

bool Tiled2dMapVectorRasterTile::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return false;
    }
    
    double zoomIdentifier = layerConfig->getZoomIdentifier(camera->getZoom());
    if (!mapInterface->is3d()) {
        zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.tileInfo.zoomIdentifier);
    }

    auto rasterDescription = std::static_pointer_cast<RasterVectorLayerDescription>(description);
    bool inZoomRange = (rasterDescription->maxZoom >= zoomIdentifier || zoomInfo.overzoom) && (rasterDescription->minZoom <= zoomIdentifier || zoomInfo.underzoom);

    if (inZoomRange != isVisible) {
        isVisible = inZoomRange;
        assert(tileObject);
        tileObject->getRenderObject()->setHidden(!inZoomRange);
    }

    if (!inZoomRange) {
        return false;
    }

    if (lastZoom &&
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant)
        && lastAlpha == alpha &&
        !isStyleStateDependant) {
        return false;
    }
    lastZoom = zoomIdentifier;
    lastAlpha = alpha;

    const EvaluationContext evalContext(zoomIdentifier, dpFactor, nullptr, featureStateManager);
    auto rasterStyle = rasterDescription->style.getRasterStyle(evalContext);
    if (auto lookupStyle = getTextureLookupStyle(rasterStyle, zoomIdentifier)) {
        rasterStyle = *lookupStyle;
    }

    if(rasterStyle == lastStyle) {
        return false;
    }
    tileObject->setStyle(rasterStyle);
    lastStyle = rasterStyle;
    return false;
}

void Tiled2dMapVectorRasterTile::clear() {
    tileObject->getGraphicsObject()->clear();
}

void Tiled2dMapVectorRasterTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    const auto &renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }
    tileObject->getGraphicsObject()->setup(renderingContext);
    if (auto lookup = resolveLookupTexture()) {
        tileObject->getQuadObject()->loadDualTexture(renderingContext, tileData, lookup->first);
    } else {
        tileObject->getQuadObject()->loadTexture(renderingContext, tileData);
    }

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorRasterTile::pause() {
    tileObject->getGraphicsObject()->pause();
}

void Tiled2dMapVectorRasterTile::resume() {
    auto mapInterface = this->mapInterface.lock();
    const auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }
    tileObject->getGraphicsObject()->resume(renderingContext);
    
    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorRasterTile::setAlpha(float alpha) {
    Tiled2dMapVectorTile::setAlpha(alpha);
}

float Tiled2dMapVectorRasterTile::getAlpha() {
    return Tiled2dMapVectorTile::getAlpha();
}

void Tiled2dMapVectorRasterTile::setRasterTileData(const Tiled2dMapVectorTileDataRaster &tileData) {

    if (!mapInterface.lock()) {
        return;
    }

    this->tileData = tileData;

#ifdef __APPLE__
    setupTile(tileData);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorRasterTile::setupTile), tileData);
#endif
}


void Tiled2dMapVectorRasterTile::setupTile(const Tiled2dMapVectorTileDataRaster tileData) {
    tileObject->getQuadObject()->removeTexture();

    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    auto graphicsObject = tileObject->getGraphicsObject();
    if (!graphicsObject->isReady()) {
        graphicsObject->setup(renderingContext);
    }

    if (auto lookup = resolveLookupTexture()) {
        tileObject->getQuadObject()->loadDualTexture(renderingContext, tileData, lookup->first);
    } else {
        tileObject->getQuadObject()->loadTexture(renderingContext, tileData);
    }

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorRasterTile::setSpriteData(const std::string &spriteId,
                                               const std::shared_ptr<SpriteData> &spriteData,
                                               const std::shared_ptr<TextureHolderInterface> &spriteTexture) {
    sprites[spriteId] = {spriteData, spriteTexture};
    auto rasterDescription = std::static_pointer_cast<RasterVectorLayerDescription>(description);
    if (!rasterDescription->textureLookup) {
        return;
    }

    const auto lookupSpriteId = rasterDescription->textureLookup->sprite.sheet.empty() ? "default" : rasterDescription->textureLookup->sprite.sheet;
    if (spriteId != lookupSpriteId) {
        return;
    }

    if (tileData) {
        lastZoom = std::nullopt;
        lastStyle = std::nullopt;
        setupTile(tileData);
        update();
    }
}

std::optional<std::pair<std::shared_ptr<TextureHolderInterface>, RectD>> Tiled2dMapVectorRasterTile::resolveLookupTexture() {
    auto rasterDescription = std::static_pointer_cast<RasterVectorLayerDescription>(description);
    if (!rasterDescription->textureLookup) {
        return std::nullopt;
    }

    auto spriteId = rasterDescription->textureLookup->sprite.sheet.empty() ? "default" : rasterDescription->textureLookup->sprite.sheet;
    const auto spriteSheet = sprites.find(spriteId);
    if (spriteSheet == sprites.end() || !spriteSheet->second.first || !spriteSheet->second.second) {
        LogWarning << "Unable to find raster lookup sprite sheet " <<= spriteId;
        return std::nullopt;
    }

    const auto &spriteName = rasterDescription->textureLookup->sprite.icon;
    const auto spriteIt = spriteSheet->second.first->sprites.find(spriteName);
    if (spriteIt == spriteSheet->second.first->sprites.end()) {
        LogError << "Unable to find raster lookup sprite " <<= spriteName;
        return std::nullopt;
    }

    const auto &sprite = spriteIt->second;
    const auto &texture = spriteSheet->second.second;
    const double width = std::max(1, sprite.width);
    const double height = std::max(1, sprite.height);
    // Lookup textures are sampled directly in the shader. On Android the backing GPU
    // texture can be padded, so normalize by texture size instead of logical image size.
    const double textureWidth = std::max(1, texture->getTextureWidth());
    const double textureHeight = std::max(1, texture->getTextureHeight());
    const RectD uvRect((sprite.x + 0.5) / textureWidth,
                       (sprite.y + 0.5) / textureHeight,
                       std::max(0.0, width - 1.0) / textureWidth,
                       std::max(0.0, height - 1.0) / textureHeight);
    return std::make_pair(texture, uvRect);
}

std::optional<RasterShaderStyle> Tiled2dMapVectorRasterTile::getTextureLookupStyle(const RasterShaderStyle &baseStyle, double zoomIdentifier) {
    auto rasterDescription = std::static_pointer_cast<RasterVectorLayerDescription>(description);
    if (!rasterDescription->textureLookup) {
        return std::nullopt;
    }

    auto lookup = resolveLookupTexture();
    if (!lookup) {
        return std::nullopt;
    }

    const auto &config = *rasterDescription->textureLookup;
    const double zoomRange = config.zoomMax - config.zoomMin;
    if (zoomRange == 0.0) {
        return std::nullopt;
    }

    const double lookupY = std::clamp((zoomIdentifier - config.zoomMin) / zoomRange, 0.0, 1.0);
    const auto &uv = lookup->second;

    // brightnessMin < 0 is an internal sentinel interpreted by platform raster shaders as texture lookup mode.
    // -1: mono lookup, -2: dual lookup, -3: quad lookup.
    const float lookupMode = config.mode == RasterTextureLookupMode::QUAD
                                 ? -3.0f
                                 : (config.mode == RasterTextureLookupMode::DUAL ? -2.0f : -1.0f);
    return RasterShaderStyle(baseStyle.opacity,
                             lookupMode,
                             (float) uv.x,
                             (float) uv.y,
                             (float) uv.width,
                             (float) uv.height,
                             (float) lookupY);
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorRasterTile::generateRenderObjects() {
    return {tileObject->getRenderObject()};
}

bool Tiled2dMapVectorRasterTile::performClick(const Coord &coord) {
    return false;
}
