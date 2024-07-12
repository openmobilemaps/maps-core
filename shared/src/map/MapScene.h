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

#include "LayerReadyState.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "SchedulerGraphicsTaskCallbacks.h"
#include "Scene.h"
#include <map>
#include <mutex>

class MapScene : public MapInterface, public SceneCallbackInterface, public SchedulerGraphicsTaskCallbacks, public std::enable_shared_from_this<MapScene> {
  public:
    MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig &mapConfig,
             const std::shared_ptr<::SchedulerInterface> &scheduler, float pixelDensity);

    virtual std::shared_ptr<::GraphicsObjectFactoryInterface> getGraphicsObjectFactory() override;

    virtual std::shared_ptr<::ShaderFactoryInterface> getShaderFactory() override;

    virtual std::shared_ptr<::SchedulerInterface> getScheduler() override;

    virtual std::shared_ptr<::RenderingContextInterface> getRenderingContext() override;

    virtual MapConfig getMapConfig() override;

    virtual std::shared_ptr<::CoordinateConversionHelperInterface> getCoordinateConverterHelper() override;

    virtual void setCallbackHandler(const std::shared_ptr<MapCallbackInterface> &callbackInterface) override;

    virtual void setCamera(const std::shared_ptr<::MapCamera2dInterface> &camera) override;

    virtual std::shared_ptr<::MapCamera2dInterface> getCamera() override;

    virtual void setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> &touchHandler) override;

    virtual std::shared_ptr<::TouchHandlerInterface> getTouchHandler() override;

    virtual std::vector<std::shared_ptr<LayerInterface>> getLayers() override;

    std::vector<std::shared_ptr<IndexedLayerInterface>> getLayersIndexed() override;

    virtual void addLayer(const std::shared_ptr<::LayerInterface> &layer) override;

    virtual void insertLayerAt(const std::shared_ptr<LayerInterface> &layer, int32_t atIndex) override;

    virtual void insertLayerAbove(const std::shared_ptr<LayerInterface> &layer,
                                  const std::shared_ptr<LayerInterface> &above) override;

    virtual void insertLayerBelow(const std::shared_ptr<LayerInterface> &layer,
                                  const std::shared_ptr<LayerInterface> &below) override;

    virtual void removeLayer(const std::shared_ptr<::LayerInterface> &layer) override;

    virtual void setViewportSize(const ::Vec2I &size) override;

    virtual void setBackgroundColor(const Color &color) override;

    virtual void invalidate() override;

    virtual void drawFrame() override;

    virtual void prepare() override;

    virtual void compute() override;

    virtual void resume() override;

    virtual void pause() override;

    virtual void destroy() override;

    virtual void drawReadyFrame(const ::RectCoord &bounds, float timeout,
                                const std::shared_ptr<MapReadyCallbackInterface> &callbacks) override;

    virtual void forceReload() override;

    virtual void requestGraphicsTaskExecution() override;

  private:
    LayerReadyState getLayersReadyState();

  private:
    const MapConfig mapConfig;

    std::shared_ptr<MapCallbackInterface> callbackHandler;

    std::shared_ptr<SchedulerInterface> scheduler;

    std::shared_ptr<SceneInterface> scene;

    std::shared_ptr<MapCamera2dInterface> camera;

    std::recursive_mutex layersMutex;
    std::map<int, std::shared_ptr<LayerInterface>> layers;

    std::shared_ptr<TouchHandlerInterface> touchHandler;

    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    bool isResumed = false;
    std::atomic_flag isInvalidated = ATOMIC_FLAG_INIT;
};
