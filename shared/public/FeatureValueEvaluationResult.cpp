/*
 * Copyright (c) 2025 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <vector>
#include <string>
#include <cstdint>
#include "Color.h"
#include "SymbolAlignment.h"
#include "BlendMode.h"
#include "FormattedStringEntry.h"
#include "TextTransform.h"
#include "Anchor.h"
#include "TextJustify.h"
#include "TextSymbolPlacement.h"
#include "IconTextFit.h"
#include "SymbolZOrder.h"
#include "LineCapType.h"

#include "FeatureValueEvaluationResult.h"
#include "FeatureValueEvaluationResult.inl"

template struct FeatureValueEvaluationResult<double>;
template struct FeatureValueEvaluationResult<Color>;
template struct FeatureValueEvaluationResult<bool>;
template struct FeatureValueEvaluationResult<std::string>;
template struct FeatureValueEvaluationResult<SymbolAlignment>;
template struct FeatureValueEvaluationResult<Vec2F>;
template struct FeatureValueEvaluationResult<std::vector<std::string>>;
template struct FeatureValueEvaluationResult<BlendMode>;
template struct FeatureValueEvaluationResult<std::vector<FormattedStringEntry>>;
template struct FeatureValueEvaluationResult<TextTransform>;
template struct FeatureValueEvaluationResult<Anchor>;
template struct FeatureValueEvaluationResult<TextJustify>;
template struct FeatureValueEvaluationResult<TextSymbolPlacement>;
template struct FeatureValueEvaluationResult<std::vector<Anchor>>;
template struct FeatureValueEvaluationResult<int64_t>;
template struct FeatureValueEvaluationResult<IconTextFit>;
template struct FeatureValueEvaluationResult<std::vector<float>>;
template struct FeatureValueEvaluationResult<SymbolZOrder>;
template struct FeatureValueEvaluationResult<LineCapType>;
