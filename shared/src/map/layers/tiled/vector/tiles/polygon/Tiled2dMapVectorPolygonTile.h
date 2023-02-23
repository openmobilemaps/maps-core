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

#include "Tiled2dMapVectorTile.h"

class Tiled2dMapVectorPolygonTile : public Tiled2dMapVectorTile {
public:
    using Tiled2dMapVectorTile::Tiled2dMapVectorTile;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setAlpha(float alpha) override;

    virtual float getAlpha() override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual void setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                             const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    virtual void updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) override;

private:
};
