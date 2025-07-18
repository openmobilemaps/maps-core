/*
 * Copyright (c) 2025 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "ZoomRange.h"
#include "Vec2F.h"

class EvaluationContext;

enum class ReevaluationPolicy : int {
    NEVER,
    ZOOM,
    STATE,
    ZOOM_AND_STATE,
    ALWAYS
};

template<typename T>
struct FeatureValueEvaluationResult {
    T value;

private:
    ReevaluationPolicy policy = ReevaluationPolicy::ALWAYS;
    int32_t stateId = -1;
    ZoomEvaluation zoomEvaluation = ZoomEvaluation();

public:
    bool isReevaluationNeeded(const EvaluationContext& context) const;
    void invalidate();

    // Convenience constructors default to ALWAYS
    FeatureValueEvaluationResult(const T& val);
    FeatureValueEvaluationResult(T&& val);

    // Static factory methods
    static FeatureValueEvaluationResult constant(const T& val);
    static FeatureValueEvaluationResult constant(T&& val);
    static FeatureValueEvaluationResult always(T&& val);
    static FeatureValueEvaluationResult zoomOnly(T&& val, const ZoomRange& range, double zoom);
    static FeatureValueEvaluationResult stateOnly(T&& val, int32_t stateId);
    static FeatureValueEvaluationResult zoomAndState(T&& val, int32_t stateId, const ZoomRange& range, double zoom);

private:
    FeatureValueEvaluationResult(const T& val, ReevaluationPolicy policy, int32_t stateId, const ZoomEvaluation& zoomEvaluation);
    FeatureValueEvaluationResult(T&& val, ReevaluationPolicy policy, int32_t stateId, const ZoomEvaluation& zoomEvaluation);
};

extern template struct FeatureValueEvaluationResult<double>;
extern template struct FeatureValueEvaluationResult<Color>;
extern template struct FeatureValueEvaluationResult<bool>;
extern template struct FeatureValueEvaluationResult<std::string>;
extern template struct FeatureValueEvaluationResult<SymbolAlignment>;
extern template struct FeatureValueEvaluationResult<Vec2F>;
extern template struct FeatureValueEvaluationResult<std::vector<std::string>>;
extern template struct FeatureValueEvaluationResult<BlendMode>;
extern template struct FeatureValueEvaluationResult<std::vector<std::string>>;
extern template struct FeatureValueEvaluationResult<std::vector<FormattedStringEntry>>;
extern template struct FeatureValueEvaluationResult<TextTransform>;
extern template struct FeatureValueEvaluationResult<Anchor>;
extern template struct FeatureValueEvaluationResult<TextJustify>;
extern template struct FeatureValueEvaluationResult<TextSymbolPlacement>;
extern template struct FeatureValueEvaluationResult<std::vector<Anchor>>;
extern template struct FeatureValueEvaluationResult<int64_t>;
extern template struct FeatureValueEvaluationResult<IconTextFit>;
extern template struct FeatureValueEvaluationResult<std::vector<float>>;
extern template struct FeatureValueEvaluationResult<SymbolZOrder>;
extern template struct FeatureValueEvaluationResult<LineCapType>;
