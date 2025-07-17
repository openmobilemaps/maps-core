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

#include "json.h"

#include <memory>
#include <string>
#include <unordered_set>

class Tiled2dMapVectorStyleParser {
public:
    static const std::string literalExpression;
    static const std::string getExpression;
    static const std::string hasExpression;
    static const std::string hasNotExpression;
    static const std::string inExpression;
    static const std::string notInExpression;
    static const std::unordered_set<std::string> compareExpression;
    static const std::unordered_set<std::string> mathExpression;
    static const std::string allExpression;
    static const std::string anyExpression;
    static const std::string caseExpression;
    static const std::string matchExpression;
    static const std::string toStringExpression;
    static const std::string toBooleanExpression;
    static const std::string toNumberExpression;
    static const std::string stopsExpression;
    static const std::string stepExpression;
    static const std::string interpolateExpression;
    static const std::string formatExpression;
    static const std::string numberFormatExpression;
    static const std::string concatExpression;
    static const std::string lengthExpression;
    static const std::string notExpression;
    static const std::string zoomExpression;
    static const std::string booleanExpression;
    static const std::string featureStateExpression;
    static const std::string globalStateExpression;
    static const std::string coalesceExpression;

    std::shared_ptr<Value> parseValue(nlohmann::json json);
    ValueVariant getVariant(const nlohmann::json &json);
};
