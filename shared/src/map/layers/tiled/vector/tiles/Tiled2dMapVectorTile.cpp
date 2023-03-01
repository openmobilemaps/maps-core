/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorTile.h"
#include "CoordinateSystemIdentifiers.h"
#include "QuadCoord.h"
#include "RenderPass.h"
#include "RenderObject.h"

Tiled2dMapVectorTile::Tiled2dMapVectorTile(const std::weak_ptr<MapInterface> &mapInterface, const Tiled2dMapTileInfo &tileInfo,
                                           const WeakActor<Tiled2dMapVectorLayer> &vectorLayer)
        : mapInterface(mapInterface), tileInfo(tileInfo), vectorLayer(vectorLayer) {
    auto mapI = mapInterface.lock();
    if (mapI) {
        debugShader = mapI->getShaderFactory()->createColorShader();
        debugQuad = mapI->getGraphicsObjectFactory()->createQuad(debugShader->asShaderProgramInterface());
        QuadCoord rendQuad = mapI->getCoordinateConverterHelper()
                ->convertQuad(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                              QuadCoord(tileInfo.bounds.topLeft,
                                        Coord(tileInfo.bounds.topLeft.systemIdentifier,
                                              tileInfo.bounds.bottomRight.x,
                                              tileInfo.bounds.topLeft.y, 0.0),
                                        tileInfo.bounds.bottomRight,
                                        Coord(tileInfo.bounds.topLeft.systemIdentifier,
                                              tileInfo.bounds.topLeft.x,
                                              tileInfo.bounds.bottomRight.y, 0.0)));
        debugQuad->setFrame(Quad2dD(Vec2D(rendQuad.topLeft.x, rendQuad.topLeft.y),
                                    Vec2D(rendQuad.topRight.x, rendQuad.topRight.y),
                                    Vec2D(rendQuad.bottomRight.x, rendQuad.bottomRight.y),
                                    Vec2D(rendQuad.bottomLeft.x, rendQuad.bottomLeft.y)),
                            RectD(0.0, 0.0, 1.0, 1.0));
        debugShader->setColor(0.5, 0.5, 0.5, 0.1);

        auto newObject = debugQuad;
        mapI->getScheduler()->addTask(std::make_shared<LambdaTask>(
                TaskConfig("Tiled2dMapVectorTile_DebugSetup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                [mapInterface, newObject] {
                    auto mapI = mapInterface.lock();
                    if (mapI && !newObject->asGraphicsObject()->isReady()) {
                        newObject->asGraphicsObject()->setup(mapI->getRenderingContext());
                    }
                }));
    }
}

void Tiled2dMapVectorTile::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }

    this->alpha = alpha;
    auto mapInterface = this->mapInterface.lock();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

float Tiled2dMapVectorTile::getAlpha() {
    return alpha;
}

void Tiled2dMapVectorTile::clear() {
    if (debugQuad) {
        debugQuad->asGraphicsObject()->clear();
    }
}

void Tiled2dMapVectorTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    if (mapInterface && debugQuad && !debugQuad->asGraphicsObject()->isReady()) {
        debugQuad->asGraphicsObject()->setup(mapInterface->getRenderingContext());
    }
}

void Tiled2dMapVectorTile::preGenerateRenderPasses() {
    std::vector<std::shared_ptr<RenderObjectInterface>> debugObjects = {std::make_shared<RenderObject>(debugQuad->asGraphicsObject())};
    debugRenderPasses = {std::make_shared<RenderPass>(RenderPassConfig(9999), debugObjects)};
}
