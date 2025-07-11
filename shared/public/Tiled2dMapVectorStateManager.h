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

#include "InternedString.h"
#include "StringInterner.h"
#include "ValueVariant.h"
#include "VectorLayerFeatureInfoValue.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <variant>
#include <atomic>

class Tiled2dMapVectorStateManager {

private:
    StringInterner &stringTable;
public:
    using FeatureState = std::unordered_map<InternedString, ValueVariant>;

    Tiled2dMapVectorStateManager(StringInterner &stringTable) : stringTable(stringTable) {};

    void setFeatureState(const std::string & identifier, const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) {
        uint64_t intIdentifier = 0;
        try {
            intIdentifier = std::stoull(identifier);
        } catch (const std::invalid_argument &e) {
            return;
        } catch (const std::out_of_range &e) {
            return;
        }

        FeatureState convertedProperties;
        convertedProperties.reserve(properties.size());
        std::transform(properties.begin(), properties.end(),
                       std::inserter(convertedProperties, convertedProperties.end()),
                       [&](const auto& entry) {
            return std::make_pair(stringTable.add(entry.first), convertToValueVariant(entry.second));
        });

        std::lock_guard<std::mutex> lock(mutex);

        featureStates.erase(std::remove_if(featureStates.begin(), featureStates.end(),
                                           [intIdentifier](const auto& item) {
            return item.first == intIdentifier;
        }), featureStates.end());

        if (!convertedProperties.empty()) {
            featureStates.emplace_back(intIdentifier, std::move(convertedProperties));
            hasNoValues = false;
        } else {
            hasNoValues = featureStates.empty() && globalState.empty();
        }

        currentState++;
    }

    const FeatureState& getFeatureState(uint64_t identifier) const {
       // XXX: dangerous optimisation -- hasNoValues can be modified
        if (hasNoValues) {
            return emptyState;
        }
        std::lock_guard<std::mutex> lock(mutex);
        auto it = std::find_if(featureStates.begin(), featureStates.end(),
                               [identifier](const auto& item) {
            return item.first == identifier;
        });

        if (it != featureStates.end()) {
            return it->second;
        }

        return emptyState;
    }

    bool empty() const {
        return hasNoValues;
    }

    void setGlobalState(const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) {
        std::lock_guard<std::mutex> lock(mutex);
        globalState.clear();
        for (const auto &property : properties) {
            globalState.emplace(stringTable.add(property.first), convertToValueVariant(property.second));
        }

        hasNoValues = properties.empty() && featureStates.empty();
        currentState++;
        globalStateId++;
    }

    ValueVariant getGlobalState(InternedString key) const {
        // XXX: dangerous optimisation -- hasNoValues can be modified
        if (hasNoValues) {
            return std::monostate();
        }

        std::lock_guard<std::mutex> lock(mutex);

        const auto &entry = globalState.find(key);
        if (entry != globalState.end()) {
            return entry->second;
        }
        return std::monostate();
    }

    int32_t getCurrentState() const {
        return currentState;
    }

private:
    std::unordered_map<InternedString, ValueVariant> globalState;
    std::vector<std::pair<uint64_t, FeatureState>> featureStates;
    mutable std::mutex mutex;
    FeatureState emptyState;

    int32_t currentState = 0;
    int32_t globalStateId = 0;

    std::atomic_bool hasNoValues = true;

    ValueVariant convertToValueVariant(const VectorLayerFeatureInfoValue &valueInfo) {
        if (valueInfo.stringVal) { return *valueInfo.stringVal; }
        else if (valueInfo.doubleVal) { return *valueInfo.doubleVal; }
        else if (valueInfo.intVal) { return *valueInfo.intVal; }
        else if (valueInfo.boolVal) { return *valueInfo.boolVal; }
        else if (valueInfo.colorVal) { return *valueInfo.colorVal; }
        else if (valueInfo.listFloatVal) { return *valueInfo.listFloatVal; }
        else if (valueInfo.listStringVal) { return *valueInfo.listStringVal; }
        else { return std::monostate(); }
    }
};
