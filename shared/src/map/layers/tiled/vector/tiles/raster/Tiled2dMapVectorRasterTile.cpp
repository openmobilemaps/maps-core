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

Tiled2dMapVectorRasterTile::Tiled2dMapVectorRasterTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                       const Tiled2dMapVersionedTileInfo &tileInfo,
                                                       const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                       const std::shared_ptr<RasterVectorLayerDescription> &description,
                                                       const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                       const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
                                                       : Tiled2dMapVectorTile(mapInterface, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager),
                                                       usedKeys(description->getUsedKeys()), zoomInfo(layerConfig->getZoomInfo()) {
    isStyleZoomDependant = usedKeys.containsUsedKey(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = usedKeys.isStateDependant();
    auto pMapInterface = mapInterface.lock();
    if (pMapInterface) {
        auto shader = pMapInterface->getShaderFactory()->createRasterShader();
        shader->asShaderProgramInterface()->setBlendMode(description->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        auto quad = pMapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
#if DEBUG
        quad->asGraphicsObject()->setDebugLabel(description->identifier);
#endif
        tileObject = std::make_shared<Textured2dLayerObject>(quad, shader, pMapInterface);
        tileObject->setRectCoord(tileInfo.tileInfo.bounds);
    }
}

void Tiled2dMapVectorRasterTile::updateRasterLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                        const Tiled2dMapVectorTileDataRaster &tileData) {
    Tiled2dMapVectorTile::updateRasterLayerDescription(description, tileData);
    isStyleZoomDependant = usedKeys.containsUsedKey(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = usedKeys.isStateDependant();
    lastZoom = std::nullopt;
    setRasterTileData(tileData);
}

void Tiled2dMapVectorRasterTile::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }
    
    double zoomIdentifier = layerConfig->getZoomIdentifier(camera->getZoom());
    zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.tileInfo.zoomIdentifier);

    auto rasterDescription = std::static_pointer_cast<RasterVectorLayerDescription>(description);
    bool inZoomRange = (rasterDescription->maxZoom >= zoomIdentifier || zoomInfo.overzoom) && (rasterDescription->minZoom <= zoomIdentifier || zoomInfo.underzoom);

    if (lastZoom &&
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant)
        && lastAlpha == alpha &&
        (lastInZoomRange && *lastInZoomRange == inZoomRange) &&
        !isStyleStateDependant) {
        return;
    }
    lastZoom = zoomIdentifier;
    lastAlpha = alpha;
    lastInZoomRange = inZoomRange;

    const EvaluationContext evalContext(zoomIdentifier, dpFactor, std::make_shared<FeatureContext>(), featureStateManager);
    auto rasterStyle = rasterDescription->style.getRasterStyle(evalContext);
    
    if (!inZoomRange) {
        rasterStyle.opacity = 0.0;
    }
    
    if(rasterStyle == lastStyle) {
        return;
    }
    tileObject->setStyle(rasterStyle);
    lastStyle = rasterStyle;
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
    tileObject->getQuadObject()->loadTexture(renderingContext, tileData);

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
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
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorRasterTile::setupTile, tileData);
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

    tileObject->getQuadObject()->loadTexture(renderingContext, tileData);

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorRasterTile::generateRenderObjects() {
    return {tileObject->getRenderObject()};
}

bool Tiled2dMapVectorRasterTile::performClick(const Coord &coord) {
    return false;
}
