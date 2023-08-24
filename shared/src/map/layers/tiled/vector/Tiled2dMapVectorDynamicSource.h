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

#include "Actor.h"
#include "Tiled2dMapVectorSourceListener.h"
#include "Tiled2dMapVectorSource.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Logger.h"
#include "Tiled2dMapVectorSource.h"

class Tiled2dMapVectorDynamicSource: public ActorObject,
public Tiled2dMapVectorSourceInterface,
public Tiled2dMapSourceReadyInterface {
public:
    Tiled2dMapVectorDynamicSource(const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                                  std::unordered_map<std::string, Actor<Tiled2dMapVectorSource>> vectorTileSources) {
        featureMap = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>>>();
    }

    void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
        if (dynamicFeatureIdentifiers.empty()) {
            return;
        }

        for (const auto &tile: currentTileInfos) {
            for (const auto &[layerName, features]: *tile.layerFeatureMaps) {
                std::vector<Tiled2dMapVectorTileInfo::FeatureTuple> newFeatures;

                for (const auto &[context, geometry] : *features) {
                    if (dynamicFeatureIdentifiers.count(context->identifier) != 0) {
                        LogDebug <<= "Found feature!!!";
                        newFeatures.push_back(std::make_tuple(context, geometry));
                    }
                }
                if (!newFeatures.empty()) {
//                    features = std::make_shared<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>(newFeatures);
                }
            }
        }
    }

    void addDynamicFeature(const std::string & identifier) {
        uint64_t intIdentifier = 0;
        try {
            intIdentifier = std::stoull(identifier);
        } catch (const std::invalid_argument &e) {
            return;
        } catch (const std::out_of_range &e) {
            return;
        }
        dynamicFeatureIdentifiers.insert(intIdentifier);
    }

    void removeDynamicFeature(const std::string & identifier) {
        uint64_t intIdentifier = 0;
        try {
            intIdentifier = std::stoull(identifier);
        } catch (const std::invalid_argument &e) {
            return;
        } catch (const std::out_of_range &e) {
            return;
        }
        dynamicFeatureIdentifiers.erase(intIdentifier);

        for (auto featureMapIt = featureMap->begin(); featureMapIt != featureMap->end(); featureMapIt++) {
            auto &[sourceLayer, features] = *featureMapIt;
            std::vector<Tiled2dMapVectorTileInfo::FeatureTuple> newFeatures; // Create a new vector to hold non-matching features

            for (const auto &[context, geometry] : *features) {
                if (context->identifier != intIdentifier) {
                    newFeatures.push_back(std::make_tuple(context, geometry)); // Keep non-matching features
                }
            }

            if (!newFeatures.empty()) {
                features = std::make_shared<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>(newFeatures); // Replace the features vector
                ++featureMapIt; // Move to the next element in the map
            } else {
                featureMapIt = featureMap->erase(featureMapIt); // Remove the entry if all features were removed
            }
        }
    }

    std::unordered_set<Tiled2dMapVectorTileInfo> getCurrentTiles() override {

    }

    void setTileReady(const Tiled2dMapTileInfo &tile) override {

    }

private:
    std::unordered_set<uint64_t> dynamicFeatureIdentifiers;

    Tiled2dMapVectorTileInfo::FeatureMap featureMap;
};
