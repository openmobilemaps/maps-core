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

#include "Circle2dLayerObject.h"
#include "CircleVectorLayerDescription.h"
#include "MapInterface.h"
#include "Tiled2dMapVectorTile.h"

class Tiled2dMapVectorCircleTile
    : public Tiled2dMapVectorTile,
      public std::enable_shared_from_this<Tiled2dMapVectorCircleTile> {
public:
    Tiled2dMapVectorCircleTile(const std::weak_ptr<MapInterface> &mapInterface,
                               const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                               const Tiled2dMapVersionedTileInfo &tileInfo,
                               const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                               const std::shared_ptr<CircleVectorLayerDescription> &description,
                               const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                               const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager);

    void updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                      const Tiled2dMapVectorTileDataVector &tileData) override;

    virtual bool update() override;

    std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    void clear() override;

    void setup() override;
    void pause() override;
    void resume() override;

    void setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

    bool performClick(const Coord &coord) override;

private:
    struct CircleEntry {
        Coord coord;
        std::shared_ptr<FeatureContext> featureContext;
        bool interactable;
        std::shared_ptr<Circle2dLayerObject> circleObject;
    };

    std::optional<double> getZoomIdentifier() const;
    bool isInZoomRange(double zoomIdentifier) const;
    void updateCircleObjects(double zoomIdentifier, bool setupGraphicsObjects);
    void setupCircles();
    void clearStaleGraphicsObjects();

    std::vector<CircleEntry> circles;
    std::vector<std::shared_ptr<RenderObjectInterface>> renderObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> toClear;

    UsedKeysCollection usedKeys;
    bool isStyleZoomDependant = true;
    bool isStyleStateDependant = true;
    std::optional<double> lastZoom = std::nullopt;
    bool isVisible = true;
    float selectionSizeFactor = 1.0f;
};
