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

#include "Color.h"
#include "Logger.h"
#include "Value.h"
#include "ValueKeys.h"

#include <cstring>

const std::string Tiled2dMapVectorStyleParser::literalExpression = "literal";
const std::string Tiled2dMapVectorStyleParser::getExpression = "get";
const std::string Tiled2dMapVectorStyleParser::hasExpression = "has";
const std::string Tiled2dMapVectorStyleParser::hasNotExpression = "!has";
const std::string Tiled2dMapVectorStyleParser::inExpression = "in";
const std::string Tiled2dMapVectorStyleParser::notInExpression = "!in";
const std::unordered_set<std::string> Tiled2dMapVectorStyleParser::compareExpression = { "==", "!=", "<", "<=", ">", ">="};
const std::unordered_set<std::string> Tiled2dMapVectorStyleParser::mathExpression = { "-", "+", "/", "*", "%", "^"};
const std::string Tiled2dMapVectorStyleParser::allExpression = "all";
const std::string Tiled2dMapVectorStyleParser::anyExpression = "any";
const std::string Tiled2dMapVectorStyleParser::caseExpression = "case";
const std::string Tiled2dMapVectorStyleParser::matchExpression = "match";
const std::string Tiled2dMapVectorStyleParser::toStringExpression = "to-string";
const std::string Tiled2dMapVectorStyleParser::toBooleanExpression = "to-boolean";
const std::string Tiled2dMapVectorStyleParser::toNumberExpression = "to-number";
const std::string Tiled2dMapVectorStyleParser::stopsExpression = "stops";
const std::string Tiled2dMapVectorStyleParser::stepExpression = "step";
const std::string Tiled2dMapVectorStyleParser::interpolateExpression = "interpolate";
const std::string Tiled2dMapVectorStyleParser::formatExpression = "format";
const std::string Tiled2dMapVectorStyleParser::numberFormatExpression = "number-format";
const std::string Tiled2dMapVectorStyleParser::concatExpression = "concat";
const std::string Tiled2dMapVectorStyleParser::lengthExpression = "length";
const std::string Tiled2dMapVectorStyleParser::notExpression = "!";
const std::string Tiled2dMapVectorStyleParser::zoomExpression = "zoom";
const std::string Tiled2dMapVectorStyleParser::booleanExpression = "boolean";
const std::string Tiled2dMapVectorStyleParser::featureStateExpression = "feature-state";
const std::string Tiled2dMapVectorStyleParser::globalStateExpression = "global-state";
const std::string Tiled2dMapVectorStyleParser::coalesceExpression = "coalesce";

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
};

static bool isExpression(const nlohmann::json &json, const std::string expression) {
    return json.is_string() && toLower(json) == expression;
}

static bool isExpression(const nlohmann::json &json, const std::unordered_set<std::string> &expression) {
    return json.is_string() && expression.count(toLower(json)) != 0;
}

// Find first occurrence starting from pos of character ch where ch is not preceded by a backslash.
static size_t findNotEscaped(const std::string &s, char ch, size_t pos) {
    size_t p = s.find(ch, pos);
    while(p != std::string::npos && p > 0 && s[p-1] == '\\') {
      p = s.find(ch, p+1);
    }
    return p;
}

/**
 * Try to parse as string interpolation expression of the form
 *    "stuff{property-name}more stuff {another property name} { bla } and some literal \{ braces \}"
 *
 * @returns StringInterpolationValue or nullptr
 */
static std::shared_ptr<Value> tryParseStringInterpolationValue(const nlohmann::json &json) {
    if(!json.is_string()) {
      return nullptr;
    }

    const std::string &s = json.get_ref<const std::string&>();

    // "parts" are the string snippets between interpolation expressions.
    // "keys" are the property keys refernced in the curly braces.
    // There are always keys.size()+1 part, even if some of the parts are empty strings.
    //   part0 {key0} part1 {key1} part2
    std::vector<std::string> parts;
    std::vector<std::string> keys;

    auto partBegin = 0;
    auto exprBegin = findNotEscaped(s, '{', partBegin);
    auto exprLast = findNotEscaped(s, '}', exprBegin);

    while (exprBegin != std::string::npos && exprLast != std::string::npos) {
        parts.push_back(s.substr(partBegin, exprBegin - partBegin));

        auto keyBegin = exprBegin + 1;
        auto keyEnd = exprLast; // one after last char of key substring
        assert(keyBegin <= keyEnd);
        // trim left
        while (keyBegin < keyEnd && std::isspace(s[keyBegin])) {
            keyBegin++;
        };
        // trim right
        while (keyBegin < keyEnd && std::isspace(s[keyEnd-1])) {
            keyEnd--;
        };
        keys.push_back(s.substr(keyBegin, keyEnd - keyBegin));

        partBegin = exprLast + 1;
        exprBegin = findNotEscaped(s, '{', partBegin);
        exprLast = findNotEscaped(s, '}', exprBegin);
    }

    if (keys.empty()) {
        return nullptr;
    }
    // Add the last string part.
    parts.push_back(s.substr(partBegin, s.length() - partBegin));
    return std::make_shared<StringInterpolationValue>(keys, parts);
}

static PropertyCompareType getCompareOperator(const nlohmann::json &string) {
    if (string == "==") {
        return PropertyCompareType::EQUAL;
    } else if (string == "!=") {
        return PropertyCompareType::NOTEQUAL;
    } else if (string == "<") {
        return PropertyCompareType::LESS;
    } else if (string == "<=") {
        return PropertyCompareType::LESSEQUAL;
    } else if (string == ">") {
        return PropertyCompareType::GREATER;
    } else if (string == ">=") {
        return PropertyCompareType::GREATEREQUAL;
    }
    assert(false);
    return PropertyCompareType::EQUAL;
}

static MathOperation getMathOperation(const nlohmann::json &string) {
    if (string == "+") {
        return MathOperation::PLUS;
    } else if (string == "-") {
        return MathOperation::MINUS;
    } else if (string == "*") {
        return MathOperation::MULTIPLY;
    } else if (string == "/") {
        return MathOperation::DIVIDE;
    } else if (string == "%") {
        return MathOperation::MODULO;
    } else if (string == "^") {
        return MathOperation::POWER;
    }
    assert(false);
    return MathOperation::PLUS;
}

std::shared_ptr<Value> Tiled2dMapVectorStyleParser::parseValue(nlohmann::json json) {
    if (json.is_array()) {
        // Example: [ "literal", 3 ]
        if (json[0] == literalExpression) {
            return std::make_shared<StaticValue>(getVariant(json[1]));

            // Example: [ "get", "class" ]
        } else if (isExpression(json[0], getExpression) && json.size() == 2 && json[1].is_string()) {
            auto key = json[1].get<std::string>();
            if (json[1].is_array() && json[1][0] == "geometry-type") {
                key = ValueKeys::TYPE_KEY;
            }
            return std::make_shared<GetPropertyValue>(key);

            // Example: [ "feature-state", "hover" ]
        } else if (isExpression(json[0], featureStateExpression) && json.size() == 2 && json[1].is_string()) {
            auto key = json[1].get<std::string>();
            return std::make_shared<FeatureStateValue>(key);

            // Example: [ "global-state",  "hover" ]
        } else if (isExpression(json[0], globalStateExpression) && json.size() == 2 && json[1].is_string()) {
            auto key = json[1].get<std::string>();
            return std::make_shared<GlobalStateValue>(key);

            // Example: [ "has",  "ref" ]
            //          [ "!has", "ref"]
        } else if ((isExpression(json[0], hasExpression) || isExpression(json[0], hasNotExpression)) && json.size() == 2 && json[1].is_string()) {
            if (isExpression(json[0], hasExpression)) {
                return std::make_shared<HasPropertyValue>(json[1].get<std::string>());
            }
            else {
                return std::make_shared<HasNotPropertyValue>(json[1].get<std::string>());
            }

            // Example: [ "in", "admin_level", 2, 4 ]
            //          [ "in", ["get", "subclass"], ["literal", ["allotments", "forest", "glacier", "golf_course", "park"]]]
        } else if ((isExpression(json[0], inExpression) || isExpression(json[0], notInExpression)) && (json[1].is_string() || (json[1].is_array() && json[1][0] == getExpression))) {
            std::unordered_set<ValueVariant> values;
            std::shared_ptr<Value> dynamicValues;


            if (json[2].is_array()){
                if (json[2][0] == literalExpression && json[2][1].is_array()) {
                    for (auto it = json[2][1].begin(); it != json[2][1].end(); it++) {
                        values.insert(getVariant(*it));
                    }
                // Example:  ["in", ["get", "plz"], ["global-state", "favoritesPlz"]],
                } else if ((json[2][0] == globalStateExpression || json[2][0] == featureStateExpression ) && json[2][1].is_string()) {
                    dynamicValues = parseValue(json[2]);
                } else {
                    for (auto it = json[2].begin(); it != json[2].end(); it++) {
                        values.insert(getVariant(*it));
                    }
                }
            } else {
                for (auto it = json.begin() + 2; it != json.end(); it++) {
                    values.insert(getVariant(*it));
                }
            }

            std::string key;
            if (json[1].is_string()){
                key = json[1];
            } else {
                key = json[1][1];
            }

            if (isExpression(json[0], inExpression)) {
                return std::make_shared<InFilter>(key, values, dynamicValues);
            } else {
                return std::make_shared<NotInFilter>(key, values, dynamicValues);
            }

        // Example: [ "!=", "intermittent", 1 ]
        //          ["!=",["get","brunnel"],"tunnel"]
        } else if (isExpression(json[0], compareExpression)) {
            // MaybeGetProperty implements deprecated [ OPERATOR, key, value ] as shorthand for [ OPERATOR, ["get" key], value ]
            auto lhs = (json[1].is_string()) ? std::make_shared<MaybeGetPropertyValue>(json[1])
                                             : parseValue(json[1]);
            auto rhs = parseValue(json[2]);

            if (lhs && rhs) {
                return std::make_shared<PropertyCompareValue>(lhs, rhs, getCompareOperator(json[0]));
            }
        }

        // Example: ["all", [ "in", "admin_level", 2, 4 ], [ "!=", "maritime", 1] ]
        else if (isExpression(json[0], allExpression)) {
            std::vector<std::shared_ptr<Value>> values;
            for (auto it = json.begin() + 1; it != json.end(); it++) {
                auto const &v = parseValue(*it);
                if (v != nullptr) {
                    values.push_back(v);
                }
            }
            if (values.empty()) {
                return nullptr;
            }
            return std::make_shared<AllValue>(values);
        }

        // Example: ["any", [ "in", "admin_level", 2, 4 ], [ "!=", "maritime", 1] ]
        else if (isExpression(json[0], anyExpression)) {
            std::vector<std::shared_ptr<Value>> values;
            for (auto it = json.begin() + 1; it != json.end(); it++) {
                auto const &v = parseValue(*it);
                if (v != nullptr) {
                    values.push_back(v);
                }
            }
            return std::make_shared<AnyValue>(values);
        }

        // Example: ["case", [ "has", "name" ], 1,0 ]
        else if (isExpression(json[0], caseExpression)){
            std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases;

            for (auto it = json.begin() + 1; (it + 1) != json.end(); it += 2) {
                auto const &condition = parseValue(*it);
                auto const &value = parseValue(*(it + 1));
                cases.push_back({condition, value});
            }

            std::shared_ptr<Value> defaultValue = parseValue(*std::prev(json.end()));

            return std::make_shared<CaseValue>(cases, defaultValue);
        }

        // Example: ["match", [ "to-string", [ "get", "width" ] ], "10", 6, "9",  5, ["8","7","6"], 4, 3]
        else if (isExpression(json[0], matchExpression)){
            std::shared_ptr<Value> compareValue = parseValue(json[1]);

            std::map<std::set<ValueVariant>, std::shared_ptr<Value>> mapping;

            auto const countElements = json.size() - 3;
            for (int i = 0; i != countElements; i += 2) {
                std::set<ValueVariant> values;
                if (json[2 + i].is_array()) {
                    for(auto const value: json[2 + i]) {
                        values.insert(getVariant(value));
                    }
                } else {
                    values.insert(getVariant(json[2 + i]));
                }

                mapping.insert({values, parseValue(json[2 + i + 1])});
            }

            std::shared_ptr<Value> defaultValue = parseValue(json[countElements + 2]);

            return std::make_shared<MatchValue>(compareValue, mapping, defaultValue);
        }

        // Example: [ "to-string", [ "get", "width" ] ]
        else if (isExpression(json[0], toStringExpression)) {
            auto toStringValue = std::make_shared<ToStringValue>(parseValue(json[1]));
            return toStringValue;
        }

        // Example: ["to-number",["get","rank"]]
        else if (isExpression(json[0], toNumberExpression)) {
            auto toNumberValue = std::make_shared<ToNumberValue>(parseValue(json[1]));
            return toNumberValue;
        }

        // Example: ["to-boolean",["get","rank"]]
        else if (isExpression(json[0], toBooleanExpression)) {
            auto toBooleanValue = std::make_shared<ToBooleanValue>(parseValue(json[1]));
            return toBooleanValue;
        }

        // Example: [ "interpolate", ["linear"], [ "zoom" ], 13, 0.3, 15, [ "match", [ "get", "class" ], "river", 0.1, 0.3 ] ]
        else if (isExpression(json[0], interpolateExpression) && (json[1][0] == "exponential" || json[1][0] == "linear")){
            double interpolationBase = 1.0;
            if (json[1][0] == "exponential" && json[1][1].is_number()) {
                interpolationBase = json[1][1].get<float>();
            }
            std::vector<std::pair<double, std::shared_ptr<Value>>> steps;

            auto const countElements = json.size() - 3;
            for (int i = 0; i != countElements; i += 2) {
                steps.push_back({json[3 + i].get<double>(), parseValue(json[3 + i + 1])});
            }

            return std::make_shared<InterpolatedValue>(interpolationBase, steps);
        }

        // Example: [ "interpolate", ["linear"], [ "zoom" ], 13, 0.3, 15, [ "match", [ "get", "class" ], "river", 0.1, 0.3 ] ]
        else if (isExpression(json[0], interpolateExpression) && json[1][0] == "cubic-bezier" && json[1].size() == 5) {
            std::vector<std::pair<double, std::shared_ptr<Value>>> steps;

            auto const countElements = json.size() - 3;
            for (int i = 0; i != countElements; i += 2) {
                steps.push_back({json[3 + i].get<double>(), parseValue(json[3 + i + 1])});
            }

            return std::make_shared<BezierInterpolatedValue>(json[1][1].get<double>(), json[1][2].get<double>(), json[1][3].get<double>(), json[1][4].get<double>(), steps);
        }

        // Example: ["format",["get","name:latin"],{},"\n",{},["get","ele"],{"font-scale":0.75}]
        // OR [ "format",["get", "name:latin"], "\n", ["get", "ele"], {"font-scale": 0.75} ]
        else if (isExpression(json[0], formatExpression)) {
            std::vector<FormatValueWrapper> values;

            for (auto it = json.begin() + 1; it != json.end(); it += 1) {
                auto const &value = parseValue(*it);
                float scale = 1.0;
                if (it + 1 != json.end() && (it + 1)->is_object()) {
                    for (auto const &[key, value] : (it + 1)->items()) {
                        if (key == "font-scale") {
                            scale = value.get<float>();
                        }
                    }
                    it += 1;
                }


                values.push_back({value, scale});
            }

            return std::make_shared<FormatValue>(values);
        }

        // Example: ["number-format",["get","temperature"], { "min-fraction-digits": 1, "max-fraction-digits": 1}]
        else if (isExpression(json[0], numberFormatExpression)) {
            const auto &value = parseValue(json[1]);
            int minFractionDigits = 0;
            int maxFractionDigits = 0;
            if (json[2].is_object()) {
                minFractionDigits = json[2].value("min-fraction-digits", 0);
                maxFractionDigits = json[2].value("max-fraction-digits", 0);
            }
            return std::make_shared<NumberFormatValue>(value, minFractionDigits, maxFractionDigits);
        }
        else if (isExpression(json[0], concatExpression)) {
            std::vector<FormatValueWrapper> values;

            for (auto it = json.begin() + 1; it != json.end(); it += 1) {
                auto const &value = parseValue(*it);
                values.push_back({value, 1.0});
            }

            return std::make_shared<FormatValue>(values);
        }

        // Example: ["length",["to-string",["get","ele"]]]
        else if (isExpression(json[0], lengthExpression)) {
            return std::make_shared<LengthValue>(parseValue(json[1]));
        }

        // Example: ["!",["has","population"]]
        else if (isExpression(json[0], notExpression)) {
            return std::make_shared<LogOpValue>(LogOpType::NOT, parseValue(json[1]));
        }

        // Example: ["step",["zoom"],"circle_black_4",6,"circle_black_4",8,"circle_black_6",10,"circle_black_8",12,"circle_black_10"]
        else if (isExpression(json[0], stepExpression)) {

            std::shared_ptr<Value> compareValue;
            // [ "zoom" ] expression can only occur here or in interpolate
            if (json[1].is_array() && isExpression(json[1][0], zoomExpression)) {
                compareValue = std::make_shared<ZoomValue>();
            } else {
                compareValue = parseValue(json[1]);
            }
            std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops;
            std::shared_ptr<Value> defaultValue = parseValue(json[2]);

            for (auto it = json.begin() + 3; (it + 1) < json.end(); it += 2) {
                auto const v = parseValue(*it);
                auto const value = parseValue(*(it + 1));
                stops.push_back({v, value});
            }

            return std::make_shared<StepValue>(compareValue, stops, defaultValue);
        }

        // Example: ["geometry-type"]
        else if (!json[0].is_null() && json[0].is_primitive() && json.size() == 1 && json[0] == "geometry-type") {
            return std::make_shared<GetPropertyValue>(ValueKeys::TYPE_KEY);
        }

        // Example: ["%",["to-number",["get","ele"]],100]
        else if (isExpression(json[0], mathExpression)) {
            return std::make_shared<MathValue>(parseValue(json[1]), parseValue(json[2]), getMathOperation(json[0]));
        }

        // Example: ["boolean", ["feature-state", "hover"], false]
        else if (isExpression(json[0], booleanExpression)) {
            std::vector<std::shared_ptr<Value>> values;
            for (auto it = json.begin() + 1; it != json.end(); it += 1) {
                values.push_back(parseValue(*it));
            }
            return std::make_shared<BooleanValue>(values);
        }

        // Example: [ "coalesce", ["get", "name_de"], ["get", "name_en"], ["get", "name"]]
        else if (isExpression(json[0], coalesceExpression)) {
            std::vector<std::shared_ptr<Value>> values;
            for (auto it = json.begin() + 1; it != json.end(); it += 1) {
                values.push_back(parseValue(*it));
            }
            return std::make_shared<CoalesceValue>(values);
        }

        // Example: [0.3, 1.0, 5.0]
        else if (!json[0].is_null()) {
            bool allPrimitive = true;
            for(auto& i : json) {
                if(!i.is_primitive()) {
                    allPrimitive = false;
                    break;
                }
            }

            if(allPrimitive) {
                return std::make_shared<StaticValue>(getVariant(json));
            } else {
                std::vector<std::shared_ptr<Value>> values;
                for(auto& i : json) {
                    values.push_back(parseValue(i));
                }

                return std::make_shared<ArrayValue>(values);
            }
        }
    }
    // Example: { "stops": [ [ 12, "rgba(240, 60, 60, 1)" ], [ 15, "rgba(240, 80, 85, 1)"] ] }
    else if (json.is_object() && json[stopsExpression].is_array()) {
        std::vector<std::pair<double, std::shared_ptr<Value>>> steps;

        for (auto const stop: json[stopsExpression]) {
            if (!stop[0].is_number()) {
                LogError <<= "Tiled2dMapVectorStyleParser not handled: " + json.dump();
                return nullptr;
            }
            steps.push_back({stop[0].get<double>(), parseValue(stop[1])});
        }

        return std::make_shared<InterpolatedValue>(1.0, steps);

    } else if (!json.is_null() && json.is_primitive()) {
        if (auto stringInterpolationValue = tryParseStringInterpolationValue(json)) {
            return stringInterpolationValue;
        } else {
            return std::make_shared<StaticValue>(getVariant(json));
        }
    }

    if (!json.is_null()) {
        LogError <<= "Tiled2dMapVectorStyleParser not handled: " + json.dump();
    }
    return nullptr;
}

ValueVariant Tiled2dMapVectorStyleParser::getVariant(const nlohmann::json &json) {
    if (json.is_number_float()) {
        return json.get<float>();
    } else if (json.is_number_integer()) {
        return json.get<int64_t>();
    } else if (json.is_boolean()) {
        return json.get<bool>();
    } else if (json.is_array() && json[0].is_number()) {
        auto floatVector = std::vector<float>();
        floatVector.reserve(json.size());
        for (auto const &el : json) {
            if (el.is_number()) {
                floatVector.push_back(el.get<float>());
            }
        }
        return floatVector;
    } else if (json.is_array() && json[0].is_string()) {
        auto stringVector = std::vector<std::string>();
        stringVector.reserve(json.size());
        for (auto const &el : json) {
            if (el.is_string()) {
                stringVector.push_back(el.get<std::string>());
            }
        }
        return stringVector;
    } else if (json.is_array() && json[0] == "literal") {
        return getVariant(json[1]);
    }

    if (json.is_string()) {
        auto string = json.get<std::string>();

        if (auto color = ColorUtil::fromString(string)) {
            return *color;
        }

        return string;
    }

    LogError <<= "Tiled2dMapVectorStyleParser variant not handled: " + json.dump();
    return "";
}
