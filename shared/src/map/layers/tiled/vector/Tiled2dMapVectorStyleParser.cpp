/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorStyleParser.h"

const std::string Tiled2dMapVectorStyleParser::literalExpression = "literal";
const std::string Tiled2dMapVectorStyleParser::getExpression = "get";
const std::string Tiled2dMapVectorStyleParser::hasExpression = "has";
const std::string Tiled2dMapVectorStyleParser::inExpression = "in";
const std::string Tiled2dMapVectorStyleParser::notInExpression = "!in";
const std::unordered_set<std::string> Tiled2dMapVectorStyleParser::compareExpression = { "==", "!=", "<", "<=", ">", ">="};
const std::unordered_set<std::string> Tiled2dMapVectorStyleParser::mathExpression = { "-", "+", "/", "*", "%", "^"};
const std::string Tiled2dMapVectorStyleParser::allExpression = "all";
const std::string Tiled2dMapVectorStyleParser::anyExpression = "any";
const std::string Tiled2dMapVectorStyleParser::caseExpression = "case";
const std::string Tiled2dMapVectorStyleParser::matchExpression = "match";
const std::string Tiled2dMapVectorStyleParser::toStringExpression = "to-string";
const std::string Tiled2dMapVectorStyleParser::toNumberExpression = "to-number";
const std::string Tiled2dMapVectorStyleParser::stopsExpression = "stops";
const std::string Tiled2dMapVectorStyleParser::stepExpression = "step";
const std::string Tiled2dMapVectorStyleParser::interpolateExpression = "interpolate";
const std::string Tiled2dMapVectorStyleParser::formatExpression = "format";
const std::string Tiled2dMapVectorStyleParser::concatExpression = "concat";
const std::string Tiled2dMapVectorStyleParser::lengthExpression = "length";
const std::string Tiled2dMapVectorStyleParser::notExpression = "!";
const std::string Tiled2dMapVectorStyleParser::zoomExpression = "zoom";
const std::string Tiled2dMapVectorStyleParser::booleanExpression = "boolean";
const std::string Tiled2dMapVectorStyleParser::featureStateExpression = "feature-state";
const std::string Tiled2dMapVectorStyleParser::coalesceExpression = "coalesce";
