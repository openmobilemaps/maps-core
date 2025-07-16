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
struct EvaluatedResult {
    T value;

private:
    ReevaluationPolicy policy = ReevaluationPolicy::ALWAYS;
    int32_t stateId = -1;
    ZoomRange zoomRange = ZoomRange();

public:
    bool isReevaluationNeeded(const EvaluationContext& context) const;
    void invalidate();

    // Convenience constructors default to ALWAYS
    EvaluatedResult(const T& val);
    EvaluatedResult(T&& val);

    // Static factory methods
    static EvaluatedResult constant(const T& val);
    static EvaluatedResult constant(T&& val);
    static EvaluatedResult always(T&& val);
    static EvaluatedResult zoomOnly(T&& val, const ZoomRange& range);
    static EvaluatedResult stateOnly(T&& val, int32_t stateId);
    static EvaluatedResult zoomAndState(T&& val, int32_t stateId, const ZoomRange& range);

private:
    EvaluatedResult(const T& val, ReevaluationPolicy policy, int32_t stateId, const ZoomRange& range);
    EvaluatedResult(T&& val, ReevaluationPolicy policy, int32_t stateId, const ZoomRange& range);
};

extern template struct EvaluatedResult<double>;
extern template struct EvaluatedResult<Color>;
extern template struct EvaluatedResult<bool>;
extern template struct EvaluatedResult<std::string>;
extern template struct EvaluatedResult<SymbolAlignment>;
extern template struct EvaluatedResult<Vec2F>;
extern template struct EvaluatedResult<std::vector<std::string>>;
extern template struct EvaluatedResult<BlendMode>;
extern template struct EvaluatedResult<std::vector<std::string>>;
extern template struct EvaluatedResult<std::vector<FormattedStringEntry>>;
extern template struct EvaluatedResult<TextTransform>;
extern template struct EvaluatedResult<Anchor>;
extern template struct EvaluatedResult<TextJustify>;
extern template struct EvaluatedResult<TextSymbolPlacement>;
extern template struct EvaluatedResult<std::vector<Anchor>>;
extern template struct EvaluatedResult<int64_t>;
extern template struct EvaluatedResult<IconTextFit>;
extern template struct EvaluatedResult<std::vector<float>>;
extern template struct EvaluatedResult<SymbolZOrder>;

#include "EvaluatedResult.inl"
