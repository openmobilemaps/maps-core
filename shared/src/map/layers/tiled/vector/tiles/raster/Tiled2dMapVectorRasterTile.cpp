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
                                                       const WeakActor<Tiled2dMapVectorLayerReadyInterface> &tileReadyInterface,
                                                       const std::shared_ptr<RasterVectorLayerDescription> &description)
                                                       : Tiled2dMapVectorTile(mapInterface, tileInfo, description, tileReadyInterface){
    auto pMapInterface = mapInterface.lock();
    if (pMapInterface) {
        auto shader = pMapInterface->getShaderFactory()->createAlphaShader();
        auto quad = pMapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
        tileObject = std::make_shared<Textured2dLayerObject>(quad, shader, pMapInterface);
        tileObject->setRectCoord(tileInfo.bounds);
        //LogDebug <<= "UBCM: Raster tile created " + std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y);
    }
}

void Tiled2dMapVectorRasterTile::updateLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                        const Tiled2dMapVectorTileDataVariant &tileData) {
    Tiled2dMapVectorTile::updateLayerDescription(description, tileData);
    setTileData(tileMask, tileData);
}

void Tiled2dMapVectorRasterTile::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    EvaluationContext evalContext(zoomIdentifier, FeatureContext());
    double opacity = std::static_pointer_cast<RasterVectorLayerDescription>(description)->style.getRasterOpacity(evalContext);
    tileObject->setAlpha(opacity);
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorRasterTile::buildRenderPasses() {
    return renderPasses;
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
    tileReadyInterface.message(&Tiled2dMapVectorLayerReadyInterface::tileIsReady, tileInfo);
}

void Tiled2dMapVectorRasterTile::setAlpha(float alpha) {
    Tiled2dMapVectorTile::setAlpha(alpha);
}

float Tiled2dMapVectorRasterTile::getAlpha() {
    return Tiled2dMapVectorTile::getAlpha();
}

void Tiled2dMapVectorRasterTile::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
    preGenerateRenderPasses();
}

void Tiled2dMapVectorRasterTile::setTileData(const std::shared_ptr <MaskingObjectInterface> &tileMask,
                                             const Tiled2dMapVectorTileDataVariant &tileData) {

    Tiled2dMapVectorTileDataRaster data = std::holds_alternative<Tiled2dMapVectorTileDataRaster>(tileData)
                                          ? std::get<Tiled2dMapVectorTileDataRaster>(tileData) : nullptr;

    if (!mapInterface.lock()) {
        return;
    }

    this->tileMask = tileMask;
    this->tileData = data;

#ifdef __APPLE__
    setupTile(tileData);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorRasterTile::setupTile, data);
#endif

    preGenerateRenderPasses();
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

    tileReadyInterface.message(&Tiled2dMapVectorLayerReadyInterface::tileIsReady, tileInfo);
}


void Tiled2dMapVectorRasterTile::updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) {
    this->tileMask = tileMask;
    preGenerateRenderPasses();
}

void Tiled2dMapVectorRasterTile::preGenerateRenderPasses() {
    if (!tileMask) {
        renderPasses = {};
        return;
    }

    std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(
            RenderPassConfig(description->renderPassIndex.value_or(0)),
            std::vector<std::shared_ptr<::RenderObjectInterface>>{tileObject->getRenderObject()},
            tileMask);
    renderPass->setScissoringRect(scissorRect);
    renderPasses = {renderPass};
}
