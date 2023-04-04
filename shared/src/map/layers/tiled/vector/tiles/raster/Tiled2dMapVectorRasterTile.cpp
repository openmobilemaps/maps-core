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
#include "MapCamera2dInterface.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "AlphaShaderInterface.h"
#include "RenderPass.h"

Tiled2dMapVectorRasterTile::Tiled2dMapVectorRasterTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                       const Tiled2dMapTileInfo &tileInfo,
                                                       const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                       const std::shared_ptr<RasterVectorLayerDescription> &description)
                                                       : Tiled2dMapVectorTile(mapInterface, tileInfo, description, tileCallbackInterface){
    auto pMapInterface = mapInterface.lock();
    if (pMapInterface) {
        auto shader = pMapInterface->getShaderFactory()->createAlphaShader();
        auto quad = pMapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
        tileObject = std::make_shared<Textured2dLayerObject>(quad, shader, pMapInterface);
        tileObject->setRectCoord(tileInfo.bounds);
    }
}

void Tiled2dMapVectorRasterTile::updateRasterLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                        const Tiled2dMapVectorTileDataRaster &tileData) {
    Tiled2dMapVectorTile::updateRasterLayerDescription(description, tileData);
    setRasterTileData(tileData);
}

void Tiled2dMapVectorRasterTile::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.zoomIdentifier);

    EvaluationContext evalContext(zoomIdentifier, FeatureContext());
    double opacity = std::static_pointer_cast<RasterVectorLayerDescription>(description)->style.getRasterOpacity(evalContext);
    tileObject->setAlpha(opacity);
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
