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

#include "Tiled2dMapVectorSubLayer.h"
#include "SymbolVectorLayerDescription.h"
#include "RenderVerticesDescription.h"
#include "LineInfo.h"
#include "Coord.h"
#include "PolygonGroup2dInterface.h"
#include "TextShaderInterface.h"
#include "TextLayerObject.h"
#include "TextInfo.h"
#include "FontLoaderInterface.h"
#include "FontLoaderResult.h"

class Tiled2dMapVectorSymbolSubLayer : public Tiled2dMapVectorSubLayer,
                                     public std::enable_shared_from_this<Tiled2dMapVectorSymbolSubLayer> {
public:
    Tiled2dMapVectorSymbolSubLayer(const std::shared_ptr<FontLoaderInterface> &fontLoader, const std::shared_ptr<SymbolVectorLayerDescription> &description);

    ~Tiled2dMapVectorSymbolSubLayer() {}

    virtual void update() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask, const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo) override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) override;

protected:
    void addTexts(const Tiled2dMapTileInfo &tileInfo, std::vector<std::shared_ptr<TextInfoInterface>> &texts);

    void setupTexts(const Tiled2dMapTileInfo &tileInfo,
                    const std::vector<std::tuple<std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>>> &texts);

    FontLoaderResult loadFont(const Font &font);


private:
    std::shared_ptr<FontLoaderInterface> fontLoader;
    std::shared_ptr<SymbolVectorLayerDescription> description;
    std::shared_ptr<TextShaderInterface> shader;


    std::recursive_mutex fontResultsMutex;
    std::unordered_map<std::string, FontLoaderResult> fontLoaderResults;
    std::recursive_mutex symbolMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>>>> tileTextMap;

    std::recursive_mutex featureGroupsMutex;
    std::vector<FeatureContext> featureGroups;

};
