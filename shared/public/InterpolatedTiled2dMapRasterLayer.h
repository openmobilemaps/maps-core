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

#include "LayerInterface.h"
#include "Textured2dLayerObject.h"
#include "Tiled2dMapRasterLayer.h"
#include "Tiled2dMapLayer.h"
#include "Tiled2dMapRasterLayerCallbackInterface.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "Tiled2dMapRasterSource.h"
#include "Tiled2dMapRasterLayerCallbackInterface.h"
#include "PolygonMaskObject.h"
#include "ShaderProgramInterface.h"
#include "AlphaShaderInterface.h"
#include "InterpolationShaderInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Tiled2dMapLayerConfig.h"
#include <mutex>
#include <unordered_map>
#include <map>
#include <atomic>


class InterpolatedTiled2dMapRasterLayer : public Tiled2dMapRasterLayer {
public:
    InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders);

    InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                          const std::shared_ptr<::MaskingObjectInterface> &mask);

    InterpolatedTiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                      const std::vector<std::shared_ptr<::LoaderInterface>> &tileLoader,
                                      const std::shared_ptr<Tiled2dMapRasterLayerShaderFactory> & shaderFactory);

    std::vector<std::shared_ptr<RenderPassInterface>> combineRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) override;

    virtual std::vector<std::shared_ptr<RenderTargetTexture>> additionalTargets() override;

    virtual void setT(double t) override;

    virtual void setAlpha(double alpha) override;

private:


protected:

    std::shared_ptr<RenderTargetTexture> renderTargetTexture;

    std::shared_ptr<Quad2dInterface> mergedTilesQuad;

    std::shared_ptr<::ShaderProgramInterface> mergedShader;
    std::shared_ptr<::AlphaShaderInterface> mergedAlphaShader;
    std::shared_ptr<::InterpolationShaderInterface> mergedInterpolationShader;

    float tFraction = 0;

};
