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

#include "ValueVariant.h"
#include "VectorLayerFeatureInfoValue.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <variant>

class Tiled2dMapVectorFeatureStateManager {
public:
    using FeatureState = std::unordered_map<std::string, ValueVariant>;

    Tiled2dMapVectorFeatureStateManager() {};

    void setFeatureState(const std::string & identifier, const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) {
        uint64_t intIdentifier = std::stoull(identifier);
        FeatureState convertedProperties;
        convertedProperties.reserve(properties.size());
        for (const auto &entry : properties) {
            convertedProperties.emplace(entry.first, convertToValueVariant(entry.second));
        }
        std::lock_guard<std::recursive_mutex> lock(mutex);
        featureStates.emplace(intIdentifier, std::move(convertedProperties));
    }

    FeatureState getFeatureState(const uint64_t &identifier) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
         auto it = featureStates.find(identifier);
         if (it != featureStates.end()) {
             return it->second;
         }
         return empty;
    }

private:
    std::unordered_map<uint64_t, FeatureState> featureStates;
    std::recursive_mutex mutex;
    FeatureState empty;

    ValueVariant convertToValueVariant(const VectorLayerFeatureInfoValue &valueInfo) {
        if (valueInfo.stringValue) { return *valueInfo.stringValue; }
        else if (valueInfo.doubleValue) { return *valueInfo.doubleValue; }
        else if (valueInfo.intValue) { return *valueInfo.intValue; }
        else if (valueInfo.boolValue) { return *valueInfo.boolValue; }
        else if (valueInfo.colorValue) { return *valueInfo.colorValue; }
        else if (valueInfo.listFloatValue) { return *valueInfo.listFloatValue; }
        else if (valueInfo.listStringValue) { return *valueInfo.listStringValue; }
        else { return std::monostate(); }
    }
};
