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

#include "Tiled2dMapVectorLayerSelectionInterface.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorLayerParserHelper.h"


class RegionVectorLayer : public Tiled2dMapVectorLayer, public Tiled2dMapVectorLayerSelectionInterface {
public:
    RegionVectorLayer(std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                      const std::vector<std::shared_ptr<::LoaderInterface>> &loaders)
    : Tiled2dMapVectorLayer(layerConfig->getLayerName(), loaders, {}),
    regionLayerConfig(layerConfig)
    {}

    RegionVectorLayer(std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                      const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                      std::string styleJsonString)
    : Tiled2dMapVectorLayer(layerConfig->getLayerName(), loaders, {}),
    regionLayerConfig(layerConfig),
    styleJsonString(styleJsonString)
    {}


    void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override {
        Tiled2dMapVectorLayer::onAdded(mapInterface);

        setSelectionDelegate(std::dynamic_pointer_cast<Tiled2dMapVectorLayerSelectionInterface>(shared_from_this()));
    }

    std::shared_ptr<::LayerInterface> asLayerInterface() override {
        return shared_from_this();
    }

protected:
    std::shared_ptr<Tiled2dMapLayerConfig> getLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &source) override {
        return regionLayerConfig;
    }

    void loadStyleJson() override {
        if (auto styleJson = styleJsonString) {
            auto parseResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(regionLayerConfig->getLayerName(), *styleJson, 1.0, loaders);

            if (parseResult.status == LoaderStatus::OK) {
                if (errorManager) {
                    errorManager->removeError(regionLayerConfig->getLayerName());
                }
                setMapDescription(parseResult.mapDescription);
            } else {
                auto tiledLayerError = TiledLayerError(parseResult.status, parseResult.errorCode, regionLayerConfig->getLayerName(),
                                                       regionLayerConfig->getLayerName(), false, std::nullopt);
                if (errorManager) {
                    errorManager->addTiledLayerError(tiledLayerError);
                }
            }
        } else {
            Tiled2dMapVectorLayer::loadStyleJson();
        }
    }
    std::shared_ptr<Tiled2dMapLayerConfig> regionLayerConfig;
    std::optional<std::string> styleJsonString;
};
