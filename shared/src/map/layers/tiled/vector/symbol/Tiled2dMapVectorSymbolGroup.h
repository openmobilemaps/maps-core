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

#include "Tiled2dMapVectorSymbolObject.h"
#include "MapInterface.h"
#include "Tiled2dMapTileInfo.h"
#include "SymbolVectorLayerDescription.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorSymbolSubLayerPositioningWrapper.h"
#include "SymbolInfo.h"
#include "Actor.h"

class Tiled2dMapVectorSymbolGroup : public ActorObject {
public:
    Tiled2dMapVectorSymbolGroup(const std::weak_ptr<MapInterface> &mapInterface,
                                const Actor<Tiled2dMapVectorFontProvider> &fontProvider,
                                const Tiled2dMapTileInfo &tileInfo,
                                const std::string &layerIdentifier,
                                const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription);

    bool initialize(const Tiled2dMapVectorTileInfo::FeatureTuple &feature);

private:
    inline std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection);

    std::shared_ptr<Tiled2dMapVectorSymbolObject> createSymbolObject(const Tiled2dMapTileInfo &tileInfo, const std::string &layerIdentifier, const std::shared_ptr<SymbolVectorLayerDescription> &description, const std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>> &symbolInfo);

    const std::weak_ptr<MapInterface> mapInterface;
    const Tiled2dMapTileInfo tileInfo;
    const std::string layerIdentifier;
    std::shared_ptr<SymbolVectorLayerDescription> layerDescription;
    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolObject>> symbolObjects;
};
