/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorPolygonTile.h"
#include "Logger.h"

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorPolygonTile::buildRenderPasses() {

}

void Tiled2dMapVectorPolygonTile::clear() {

}

void Tiled2dMapVectorPolygonTile::setup() {

}

void Tiled2dMapVectorPolygonTile::setAlpha(float alpha) {

}

float Tiled2dMapVectorPolygonTile::getAlpha() {

}

void Tiled2dMapVectorPolygonTile::setScissorRect(const std::optional<::RectI> &scissorRect) {

}

void Tiled2dMapVectorPolygonTile::setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                 const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    LogDebug << "Received: " <<= layerFeatures.size();

    //TODO: create some object and send some messages
}

void Tiled2dMapVectorPolygonTile::updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) {

}
