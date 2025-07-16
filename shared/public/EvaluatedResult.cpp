//
//  EvaluatedResult.cpp
//  MapCore
//
//  Created by Marco Zimmermann on 16.07.2025.
//

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

#include "EvaluatedResult.h"
#include "EvaluatedResult.inl"

template struct EvaluatedResult<double>;
template struct EvaluatedResult<Color>;
template struct EvaluatedResult<bool>;
template struct EvaluatedResult<std::string>;
template struct EvaluatedResult<SymbolAlignment>;
template struct EvaluatedResult<Vec2F>;
template struct EvaluatedResult<std::vector<std::string>>;
template struct EvaluatedResult<BlendMode>;
template struct EvaluatedResult<std::vector<FormattedStringEntry>>;
template struct EvaluatedResult<TextTransform>;
template struct EvaluatedResult<Anchor>;
template struct EvaluatedResult<TextJustify>;
template struct EvaluatedResult<TextSymbolPlacement>;
template struct EvaluatedResult<std::vector<Anchor>>;
template struct EvaluatedResult<int64_t>;
template struct EvaluatedResult<IconTextFit>;
template struct EvaluatedResult<std::vector<float>>;
template struct EvaluatedResult<SymbolZOrder>;
