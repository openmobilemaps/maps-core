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

#include "Value.h"
#include "EvaluatedResult.h"

template<class ResultType>
class ValueEvaluator {
public:
    ValueEvaluator(const std::shared_ptr<Value> &value) : value(value) {
        if(!value) {
            return;
        }

        updateValue(value);
    }

    ValueEvaluator(const ValueEvaluator& evaluator) : ValueEvaluator(evaluator.getValue()) {};

    std::shared_ptr<Value> getValue() const {
        return value;
    }

    void updateValue(std::shared_ptr<Value> newValue) {
        value = newValue;

        usedKeysCollection = newValue ? std::move(newValue->getUsedKeys()) : std::move(UsedKeysCollection());

        isStatic = usedKeysCollection.empty();
        isZoomDependent = usedKeysCollection.usedKeys.contains("zoom");
        isStateDependant = usedKeysCollection.isStateDependant();
        needsReevaluation = isZoomDependent || isStateDependant;

        lastResults.clear();
        staticValue = std::nullopt;

        if(isZoomDependent) {
            getZoomRange();
        }

        usesFullZoomRange = zoomRange.isFull();
    }

    void getZoomRange() {
        zoomRange = ZoomRange();

        if(value) {
            value->evaluateZoomRange(zoomRange);
        }
    }

    EvaluatedResult<ResultType> getResult(const EvaluationContext &context, const ResultType &defaultValue) {
        if (!value) {
            return EvaluatedResult<ResultType>::constant(defaultValue);
        }

        if (isStatic) {
            if(!staticValue) {
                staticValue = value->evaluateOr(context, defaultValue);
            }

            return EvaluatedResult<ResultType>::constant(*staticValue);
        }

        if(!needsReevaluation) {
            return EvaluatedResult<ResultType>::constant(value->evaluateOr(context, defaultValue));
        }

        bool stateDependent = isStateDependant && context.featureStateManager;

        if(usesFullZoomRange) {
            return value->evaluateOr(context, defaultValue);
        }

        if(stateDependent && isZoomDependent) {
            auto currentStateId = context.featureStateManager->getCurrentState();
            return EvaluatedResult<ResultType>::zoomAndState(value->evaluateOr(context, defaultValue), currentStateId, zoomRange);
        } else if(stateDependent) {
            auto currentStateId = context.featureStateManager->getCurrentState();
            return EvaluatedResult<ResultType>::stateOnly(value->evaluateOr(context, defaultValue), currentStateId);
        } else if(isZoomDependent) {
            return EvaluatedResult<ResultType>::zoomOnly(value->evaluateOr(context, defaultValue), zoomRange);
        }

        int64_t identifier = usedKeysCollection.getHash(context);
        std::lock_guard<std::mutex> lock(mutex);

        const auto &lastResultIt = lastResults.find(identifier);
        if (lastResultIt != lastResults.end()) {
            return lastResultIt->second;
        }

        const auto &result = value->evaluateOr(context, defaultValue);
        lastResults.insert({identifier, result});

        return result;
    }

private:
    std::mutex mutex;
    std::shared_ptr<Value> value;
    UsedKeysCollection usedKeysCollection;
    std::optional<ResultType> staticValue;
    std::optional<ResultType> globalValue;

    bool isStatic = false;
    bool isZoomDependent = false;
    bool usesFullZoomRange = true;
    bool isStateDependant = false;
    bool needsReevaluation = true;

    std::unordered_map<uint64_t, ResultType> lastResults;

    ZoomRange zoomRange;
};
