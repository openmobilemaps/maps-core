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

#include "Tiled2dMapLayer.h"
#include "Tiled2dMapVectorLayerInterface.h"
#include "Tiled2dMapVectorSource.h"
#include "Tiled2dMapVectorSubLayer.h"
#include "VectorMapDescription.h"
#include "FontLoaderInterface.h"
#include "PolygonMaskObject.h"
#include "Tiled2dMapVectorLayerReadyInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Tiled2dMapVectorLayerSelectionInterface.h"

#ifdef IOS_SIGNPOST
#include <os/log.h>
#include <os/signpost.h>
#endif

class Tiled2dMapVectorLayer : public Tiled2dMapLayer, public Tiled2dMapVectorLayerInterface, public Tiled2dMapVectorLayerReadyInterface {
public:
    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::string &path,
                          const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                          const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                          double dpFactor);

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::shared_ptr<VectorMapDescription> & mapDescription,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                          const std::shared_ptr<::FontLoaderInterface> & fontLoader);

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    void forceReload() override;

    virtual void onTilesUpdated() override;

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile) override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    void setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate);

    void setSelectedFeatureIdentfier(std::optional<int64_t> identifier);

    std::shared_ptr<VectorLayerDescription> getLayerDescriptionWithIdentifier(std::string identifier);

    void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription);

protected:
    std::shared_ptr<Tiled2dMapVectorSource> vectorTileSource;

private:
    void scheduleStyleJsonLoading();

    void loadStyleJson();

    void setMapDescription(const std::shared_ptr<VectorMapDescription> &mapDescription);

    void initializeVectorLayer(const std::vector<std::shared_ptr<LayerInterface>> &newSublayers);

    virtual void updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> &toSetupMaskObject, const std::vector<const std::shared_ptr<MaskingObjectInterface>> &obsoleteMaskObjects);

    std::shared_ptr<LayerInterface> getLayerForDescription(const std::shared_ptr<VectorLayerDescription> &layerDescription);

    void loadSpriteData();

    const std::optional<double> dpFactor;

    const std::string layerName;
    std::optional<std::string> styleJsonPath;
    std::shared_ptr<VectorMapDescription> mapDescription;

    std::unordered_map<std::string, std::shared_ptr<Tiled2dMapLayerConfig>> layerConfigs;

    const std::shared_ptr<FontLoaderInterface> fontLoader;

    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;

    std::recursive_mutex tileUpdateMutex;

    std::recursive_mutex tileSetMutex;
    std::unordered_set<Tiled2dMapVectorTileInfo> tileSet;

    std::recursive_mutex tilesReadyMutex;
    std::unordered_set<Tiled2dMapTileInfo> tilesReady;

    std::recursive_mutex tilesReadyCountMutex;
    std::unordered_map<Tiled2dMapTileInfo, int> tilesReadyCount;

    std::recursive_mutex tileMaskMapMutex;
    std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> tileMaskMap;

    std::recursive_mutex sublayerMutex;
    std::vector<std::shared_ptr<LayerInterface>> sublayers;

    std::recursive_mutex sourceLayerMapMutex;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::shared_ptr<Tiled2dMapVectorSubLayer>>>> sourceLayerMap;

    std::atomic_bool isLoadingStyleJson = false;
    std::atomic_bool isResumed = false;

    std::weak_ptr<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;

#ifdef IOS_SIGNPOST
    os_log_t log_handle;
    os_signpost_id_t tileReady;
#endif
};


