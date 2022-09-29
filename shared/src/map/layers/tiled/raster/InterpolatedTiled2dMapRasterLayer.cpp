/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "InterpolatedTiled2dMapRasterLayer.h"
#include "LambdaTask.h"
#include "MapCamera2dInterface.h"
#include "MapConfig.h"
#include "RenderConfigInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "PolygonCompare.h"
#include <Logger.h>
#include <map>

InterpolatedTiled2dMapRasterLayer::InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders)
: Tiled2dMapRasterLayer(layerConfig, tileLoaders) {}

InterpolatedTiled2dMapRasterLayer::InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                             const std::shared_ptr<::MaskingObjectInterface> &mask)
: Tiled2dMapRasterLayer(layerConfig, tileLoaders, mask) {}

InterpolatedTiled2dMapRasterLayer::InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                             const std::shared_ptr<::ShaderProgramInterface> &shader)
: Tiled2dMapRasterLayer(layerConfig, tileLoaders, shader) {}

void InterpolatedTiled2dMapRasterLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    Tiled2dMapRasterLayer::onAdded(mapInterface);
    renderTargetTexture = mapInterface->createRenderTargetTexture();

    auto objectFactory = mapInterface->getGraphicsObjectFactory();
    auto shaderFactory = mapInterface->getShaderFactory();
    auto alphaShader = shaderFactory->createAlphaShader();
    std::shared_ptr<Quad2dInterface> quad = objectFactory->createQuad(shader);
    mergedTilesLayerObject = std::make_shared<Textured2dLayerObject>(quad, alphaShader, mapInterface);
}

void InterpolatedTiled2dMapRasterLayer::onRemoved() {
    Tiled2dMapRasterLayer::onRemoved();
    renderTargetTexture = nullptr;
}

void InterpolatedTiled2dMapRasterLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    Tiled2dMapRasterLayer::onVisibleBoundsChanged(visibleBounds, zoom);
    if (mergedTilesLayerObject) {
        mergedTilesLayerObject->setRectCoord(visibleBounds);
    }

}

std::vector<std::shared_ptr<RenderPassInterface>>  InterpolatedTiled2dMapRasterLayer::combineRenderPasses() {


    auto newRenderPasses = Tiled2dMapRasterLayer::generateRenderPasses(1.0, curT, renderTargetTexture);
    auto tilesNext = Tiled2dMapRasterLayer::generateRenderPasses(1.0, curT+1, renderTargetTexture);
    newRenderPasses.insert(newRenderPasses.end(), tilesNext.begin(), tilesNext.end());

    auto texture = renderTargetTexture->textureHolder();
    if (texture) {
        mergedTilesLayerObject->getQuadObject()->loadTexture(nullptr, texture);
    }

    auto renderObject = std::make_shared<RenderObject>(mergedTilesLayerObject->getQuadObject()->asGraphicsObject(), false);
    std::shared_ptr<RenderPass> finalRenderPass = std::make_shared<RenderPass>(RenderPassConfig(0, 0),
                                                                          std::vector<std::shared_ptr<::RenderObjectInterface>> { renderObject });

    newRenderPasses.push_back(finalRenderPass);

    return newRenderPasses;
}

