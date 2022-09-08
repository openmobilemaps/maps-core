/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorRasterSubLayer.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "MapCamera2dInterface.h"

Tiled2dMapVectorRasterSubLayer::Tiled2dMapVectorRasterSubLayer(const std::shared_ptr<::RasterVectorLayerDescription> &description,
                                                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders):
Tiled2dMapRasterLayer(std::make_shared<Tiled2dMapVectorRasterSubLayerConfig>(description), tileLoaders),
description(description) {}

void Tiled2dMapVectorRasterSubLayer::update() {
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    EvaluationContext evalContext(zoomIdentifier, FeatureContext());
    double opacity = description->style.getRasterOpacity(evalContext);
    setAlpha(opacity);
    Tiled2dMapRasterLayer::update();
}
