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
#include "FeatureValueEvaluationResult.h"

template<class ResultType>
class FeatureValueEvaluator {
public:
    FeatureValueEvaluator(const std::shared_ptr<Value> &value) : value(value) {
        if(!value) {
            return;
        }

        updateValue(value);
    }

    FeatureValueEvaluator(const FeatureValueEvaluator& evaluator) : FeatureValueEvaluator(evaluator.getValue()) {};

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

    FeatureValueEvaluationResult<ResultType> getResult(const EvaluationContext &context, const ResultType &defaultValue) {
        if (!value) {
            return FeatureValueEvaluationResult<ResultType>::constant(defaultValue);
        }

        if (isStatic) {
            if(!staticValue) {
                staticValue = value->evaluateOr(context, defaultValue);
            }

            return FeatureValueEvaluationResult<ResultType>::constant(*staticValue);
        }

        if(!needsReevaluation) {
            return FeatureValueEvaluationResult<ResultType>::constant(value->evaluateOr(context, defaultValue));
        }

        bool stateDependent = isStateDependant && context.featureStateManager;

        if(usesFullZoomRange) {
            return value->evaluateOr(context, defaultValue);
        }

        if(stateDependent && isZoomDependent) {
            auto currentStateId = context.featureStateManager->getCurrentState();
            return FeatureValueEvaluationResult<ResultType>::zoomAndState(value->evaluateOr(context, defaultValue), currentStateId, zoomRange, context.zoomLevel ? *context.zoomLevel : 0.0);
        } else if(stateDependent) {
            auto currentStateId = context.featureStateManager->getCurrentState();
            return FeatureValueEvaluationResult<ResultType>::stateOnly(value->evaluateOr(context, defaultValue), currentStateId);
        } else if(isZoomDependent) {
            return FeatureValueEvaluationResult<ResultType>::zoomOnly(value->evaluateOr(context, defaultValue), zoomRange, context.zoomLevel ? *context.zoomLevel : 0.0);
        }

        // shouldn't happen, one of the cases above covers this
        return value->evaluateOr(context, defaultValue);
    }

private:
    std::shared_ptr<Value> value;
    UsedKeysCollection usedKeysCollection;
    std::optional<ResultType> staticValue;
    std::optional<ResultType> globalValue;

    bool isStatic = false;
    bool isZoomDependent = false;
    bool usesFullZoomRange = true;
    bool isStateDependant = false;
    bool needsReevaluation = true;

    ZoomRange zoomRange;
};
