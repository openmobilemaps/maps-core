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
#include <atomic>

class Tiled2dMapVectorFeatureStateManager {
public:
    using FeatureState = std::unordered_map<std::string, ValueVariant>;

    Tiled2dMapVectorFeatureStateManager() {};

    void setFeatureState(const std::string & identifier, const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) {
        uint64_t intIdentifier = std::stoull(identifier);
        FeatureState convertedProperties;
        convertedProperties.reserve(properties.size());
        std::transform(properties.begin(), properties.end(),
                       std::inserter(convertedProperties, convertedProperties.end()),
                       [&](const auto& entry) {
            return std::make_pair(entry.first, convertToValueVariant(entry.second));
        });

        std::lock_guard<std::recursive_mutex> lock(mutex);

        featureStates.erase(std::remove_if(featureStates.begin(), featureStates.end(),
                                           [intIdentifier](const auto& item) {
            return item.first == intIdentifier;
        }), featureStates.end());

        if (!convertedProperties.empty()) {
            featureStates.emplace_back(intIdentifier, std::move(convertedProperties));
            hasValues.test_and_set();
        } else {
            if (featureStates.empty()) {
                hasValues.clear();
            }
        }
    }

    FeatureState getFeatureState(const uint64_t &identifier) {
        if (!hasValues.test()) {
            return emptyState;
        }
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto it = std::find_if(featureStates.begin(), featureStates.end(),
                               [identifier](const auto& item) {
            return item.first == identifier;
        });
        
        if (it != featureStates.end()) {
            return it->second;
        }

        return emptyState;
    }

    bool empty() {
        return !hasValues.test();
    }

private:
    std::vector<std::pair<uint64_t, FeatureState>> featureStates;
    std::recursive_mutex mutex;
    FeatureState emptyState;

    std::atomic_flag hasValues = ATOMIC_FLAG_INIT;

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
