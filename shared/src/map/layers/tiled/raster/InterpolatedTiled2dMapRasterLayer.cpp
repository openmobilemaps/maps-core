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
#include "Tiled2dMapRasterLayerShaderFactory.h"
#include <Logger.h>
#include <map>

InterpolatedTiled2dMapRasterLayer::InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders)
: Tiled2dMapRasterLayer(layerConfig, tileLoaders), mergedShader(nullptr) {}

InterpolatedTiled2dMapRasterLayer::InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                             const std::shared_ptr<::MaskingObjectInterface> &mask)
: Tiled2dMapRasterLayer(layerConfig, tileLoaders, mask), mergedShader(nullptr) {}

InterpolatedTiled2dMapRasterLayer::InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders, const std::shared_ptr<Tiled2dMapRasterLayerShaderFactory> & shaderFactory)
: Tiled2dMapRasterLayer(layerConfig, tileLoaders, shaderFactory) {}

void InterpolatedTiled2dMapRasterLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    Tiled2dMapRasterLayer::onAdded(mapInterface);
    renderTargetTexture = mapInterface->createRenderTargetTexture();

    auto objectFactory = mapInterface->getGraphicsObjectFactory();
    auto defaultShaderFactory = mapInterface->getShaderFactory();
    auto alphaShader = defaultShaderFactory->createAlphaShader();
    if (shaderFactory) {
        mergedInterpolationShader = shaderFactory->finalShader();
        if (mergedInterpolationShader) {
            mergedAlphaShader = mergedInterpolationShader->asAlphaShaderInterface();
        }
        if (mergedAlphaShader) {
            mergedShader = mergedAlphaShader->asShaderProgramInterface();
        }
    }
    if (!mergedShader) {
        mergedAlphaShader = defaultShaderFactory->createAlphaShader();
        mergedShader = mergedAlphaShader->asShaderProgramInterface();
    }

    mergedAlphaShader->updateAlpha(alpha);
    mergedTilesQuad = objectFactory->createQuad(mergedShader);

    mergedTilesQuad->setFrame(Quad2dD(Vec2D(-1, 1), Vec2D(1,1), Vec2D(1,-1), Vec2D(-1,-1)), RectD(0, 0, 1, 1));

    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    mergedTilesQuad->asGraphicsObject()->setup(renderingContext);

}

void InterpolatedTiled2dMapRasterLayer::onRemoved() {
    Tiled2dMapRasterLayer::onRemoved();
    renderTargetTexture = nullptr;
}

std::vector<std::shared_ptr<RenderTargetTexture>> InterpolatedTiled2dMapRasterLayer::additionalTargets() {
    return {renderTargetTexture};
}

void InterpolatedTiled2dMapRasterLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    Tiled2dMapRasterLayer::onVisibleBoundsChanged(visibleBounds, zoom);
//    if (mergedTilesLayerObject) {
//        mergedTilesLayerObject->setRectCoord(visibleBounds);
//    }

}

void InterpolatedTiled2dMapRasterLayer::setT(double t) {
    int curT = t;
    tFraction = t - (double)curT;
    Tiled2dMapRasterLayer::setT(t);
}

void InterpolatedTiled2dMapRasterLayer::setAlpha(double alpha) {
    if (alpha == this->alpha) {
        return;
    }
    mergedAlphaShader->updateAlpha(alpha);
    this->alpha = alpha;

    if (mapInterface) {
        mapInterface->invalidate();
    }
}

std::vector<std::shared_ptr<RenderPassInterface>>  InterpolatedTiled2dMapRasterLayer::combineRenderPasses() {

    auto newRenderPasses = Tiled2dMapRasterLayer::generateRenderPasses(1.0, curT+1, renderTargetTexture);
    auto tilesNext = Tiled2dMapRasterLayer::generateRenderPasses(0.0, curT , renderTargetTexture);
    newRenderPasses.insert(newRenderPasses.end(), tilesNext.begin(), tilesNext.end());

    auto texture = renderTargetTexture->textureHolder();
    if (texture) {
        auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
        mergedTilesQuad->loadTexture(renderingContext, texture);
    }

    if (mergedInterpolationShader) {
        mergedInterpolationShader->updateFraction(tFraction);
    }

    auto renderObject = std::make_shared<RenderObject>(mergedTilesQuad->asGraphicsObject(), true);
    std::shared_ptr<RenderPass> finalRenderPass = std::make_shared<RenderPass>(RenderPassConfig(0, 0),
                                                                          std::vector<std::shared_ptr<::RenderObjectInterface>> { renderObject });

    newRenderPasses.push_back(finalRenderPass);

    return newRenderPasses;
}

