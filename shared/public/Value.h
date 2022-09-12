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

#include "UnitBezier.h"
#include "Color.h"
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <vtzero/vector_tile.hpp>
#include <cmath>
#include <variant>
#include "HashedTuple.h"
#include "Vec2F.h"
#include "Anchor.h"
#include "FormattedStringEntry.h"
#include "LineCapType.h"
#include "TextTransform.h"
#include <sstream>

namespace std {
    template <>
    struct hash<Color> {
        size_t operator()(Color c) const {
            return std::hash<std::tuple<float, float, float, float>>()({c.r, c.g, c.b, c.a});
        }
    };

    template <>
    struct hash<std::vector<float>> {
        size_t operator()(std::vector<float> vector) const {
            size_t hash = 0;
            for(auto const val: vector) {
                std::hash_combine(hash, std::hash<float>{}(val));
            }
            return hash;
        }
    };

    template <>
    struct hash<std::vector<std::string>> {
        size_t operator()(std::vector<std::string> vector) const {
            size_t hash = 0;
            for(auto const val: vector) {
                std::hash_combine(hash, std::hash<std::string>{}(val));
            }
            return hash;
        }
    };

    template <>
    struct hash<std::vector<FormattedStringEntry>> {
        size_t operator()(std::vector<FormattedStringEntry> vector) const {
            size_t hash = 0;
            for(auto const val: vector) {
                std::hash_combine(hash, std::hash<std::string>{}(val.text));
                std::hash_combine(hash, std::hash<float>{}(val.scale));
            }
            return hash;
        }
    };
}

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

typedef std::variant<std::string,
                     double,
                     int64_t,
                     bool,
                     Color,
                     std::vector<float>,
                     std::vector<std::string>,
                     std::vector<FormattedStringEntry>> ValueVariant;

struct property_value_mapping : vtzero::property_value_mapping {
    using float_type = double;
    using uint_type = int64_t;
};

class FeatureContext {
    using keyType = std::string;
    using valueType = ValueVariant;
    using mapType = std::unordered_map<keyType, valueType>;
public:
    mapType propertiesMap;

    vtzero::GeomType geomType;

    FeatureContext() {}

    FeatureContext(const FeatureContext &other) :
    propertiesMap(std::move(other.propertiesMap)),
    geomType(other.geomType) {}

    FeatureContext(vtzero::feature const &feature) {
        geomType = feature.geometry_type();

        propertiesMap = vtzero::create_properties_map<mapType, keyType, valueType, property_value_mapping>(feature);

        switch (geomType) {
            case vtzero::GeomType::LINESTRING:
                propertiesMap.insert({"$type", "LineString"});
            case vtzero::GeomType::POINT:
                propertiesMap.insert({"$type", "Point"});
            case vtzero::GeomType::POLYGON:
                propertiesMap.insert({"$type", "Polygon"});
            case vtzero::GeomType::UNKNOWN:
                propertiesMap.insert({"$type", "Unknown"});
        }
    };

    bool contains(const std::string &key) const {
        return propertiesMap.find(key) != propertiesMap.end();
    }

    std::optional<ValueVariant> getValue(const std::string &key) const {
        auto it = propertiesMap.find(key);
        if(it == propertiesMap.end()) { return std::nullopt; }

        auto &variant = it->second;

        // TODO: maybe convert here to color? if string is a color
        //if (std::holds_alternative<std::string>(variant)) {
        //    return std::nullopt;
        //}

        return variant;
    }

    size_t getStyleHash(const std::unordered_set<std::string> &usedKeys) const {
        size_t hash = 0;
        for(auto const [key, val]: propertiesMap) {
            if (usedKeys.count(key) != 0) {
                std::hash_combine(hash, std::hash<valueType>{}(val));
            }
        }
        return hash;
    }

    bool operator==(const FeatureContext &o) const { return propertiesMap == o.propertiesMap; }
};


class EvaluationContext {
public:
    std::optional<double> zoomLevel;
    const FeatureContext &feature;

    EvaluationContext(std::optional<double> zoomLevel,
                      const FeatureContext &feature) : zoomLevel(zoomLevel), feature(feature) {};
};

class Value {
public:
    Value() {};
    virtual std::unordered_set<std::string> getUsedKeys() { return {}; };
    virtual ValueVariant evaluate(const EvaluationContext &context) = 0;


    template<typename T>
    T evaluateOr(const EvaluationContext &context, const T &alternative) {
        auto const value = evaluate(context);
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }

        // convert int to double
        if constexpr (std::is_same<T, double>::value) {
            if (std::holds_alternative<int64_t>(value)) {
                return (double)std::get<int64_t>(value);
            }
        }

        // convert double to int
        if constexpr (std::is_same<T, int64_t>::value) {
            if (std::holds_alternative<double>(value)) {
                return (int64_t)std::get<double>(value);
            }
        }
        return alternative;
    }

    template<>
    Vec2F evaluateOr(const EvaluationContext &context, const Vec2F &alternative) {
        auto const value = evaluateOr(context, std::vector<float>{ alternative.x, alternative.y });
        return Vec2F(value[0], value[1]);
    }

    std::optional<::Anchor> anchorFromString(const std::string &value) {
        if (value == "center") {
            return Anchor::CENTER;

        } else if (value == "left") {
            return Anchor::LEFT;

        } else if (value == "right") {
            return Anchor::RIGHT;

        } else if (value == "top") {
            return Anchor::TOP;

        } else if (value == "bottom") {
            return Anchor::BOTTOM;

        } else if (value == "top-left") {
            return Anchor::TOP_LEFT;

        } else if (value == "top-right") {
            return Anchor::TOP_RIGHT;

        } else if (value == "bottom-left") {
            return Anchor::BOTTOM_LEFT;

        } else if (value == "bottom-right") {
            return Anchor::BOTTOM_RIGHT;
        }

        return std::nullopt;
    }

    template<>
    Anchor evaluateOr(const EvaluationContext &context, const Anchor &alternative) {
        auto const value = evaluateOr(context, std::string(""));
        auto anchor = anchorFromString(value);
        if (anchor) {
            return *anchor;
        }
        return alternative;
    }

    std::optional<LineCapType> capTypeFromString(const std::string &value) {
        if (value == "butt") {
            return LineCapType::BUTT;

        } else if (value == "round") {
            return LineCapType::ROUND;

        } else if (value == "square") {
            return LineCapType::SQUARE;
        }

        return std::nullopt;
    }

    template<>
    LineCapType evaluateOr(const EvaluationContext &context, const LineCapType &alternative) {
        auto const value = evaluateOr(context, std::string(""));
        auto type = capTypeFromString(value);
        if (type) {
            return *type;
        }
        return alternative;
    }

    std::optional<TextTransform> textTransformFromString(const std::string &value) {
        if (value == "none") {
            return TextTransform::NONE;
        } else if (value == "uppercase") {
            return TextTransform::UPPERCASE;
        }
        return std::nullopt;
    }

    template<>
    TextTransform evaluateOr(const EvaluationContext &context, const TextTransform &alternative) {
        auto const value = evaluateOr(context, std::string(""));
        auto type = textTransformFromString(value);
        if (type) {
            return *type;
        }
        return alternative;
    }

    template<>
    std::vector<Anchor> evaluateOr(const EvaluationContext &context, const std::vector<Anchor> &alternative) {
        auto const values = evaluateOr(context, std::vector<std::string>());
        std::vector<Anchor> result;
        for (auto const &value: values) {
            auto anchor = anchorFromString(value);
            if (anchor) {
                result.push_back(*anchor);
            }
        }
        if (!result.empty()) {
            return result;
        }
        return alternative;
    }

};


class GetPropertyValue : public Value {
public:
    GetPropertyValue(const std::string key) : key(key) {};

    std::unordered_set<std::string> getUsedKeys() override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        if (key == "zoom" && context.zoomLevel) {
            return *context.zoomLevel;
        }
        auto result = context.feature.getValue(key);
        if (result) {
            return *result;
        }
        return "";
    };
private:
    const std::string key;
};

class ToStringValue: public Value {
    const std::shared_ptr<Value> value;

public:
    ToStringValue(const std::shared_ptr<Value> value): value(value) {}

    std::unordered_set<std::string> getUsedKeys() override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        return std::visit(overloaded {
            [](std::string val){
                return val;
            },
            [](double val){
                // strip all decimal places
                return std::to_string((int64_t)val);
            },
            [](int64_t val){
                return std::to_string(val);
            },
            [](bool val){
                return std::string("");
            },
            [](Color val){
                return std::string("");
            },
            [](std::vector<float> val){
                return std::string("");
            },
            [](std::vector<std::string> val){
                return std::string("");
            },
            [](std::vector<FormattedStringEntry> val){
                std::string string = "";
                for(auto const v: val) {
                    string += v.text;
                }
                return string;
            }
        }, value->evaluate(context));
    };
};

class StaticValue : public Value {
public:
    StaticValue(const ValueVariant value) : value(value) {};

    ValueVariant evaluate(const EvaluationContext &context) override {
        if (std::holds_alternative<std::string>(value)) {
            std::string res = std::get<std::string>(value);

            auto result = context.feature.getValue(res);
            if (result) {
                return *result;
            }

            auto begin = res.find("{");
            auto end = res.find("}", begin);

            while ( begin != std::string::npos &&
                   end != std::string::npos &&
                   end > begin &&
                   (begin == 0 || res[begin - 1] != '\\') &&
                   (end == 0 || res[end - 1] != '\\')) {

                std::string key = res.substr (begin + 1,(end - begin) - 1);

                std::string replacement = ToStringValue(std::make_shared<GetPropertyValue>(key)).evaluateOr(context, std::string());

                res.replace(begin,end + 1, replacement);

                begin = res.find("{", (begin + replacement.size()));
                end = res.find("}", begin);
            }

            return res;

        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            std::vector<std::string> res = std::get<std::vector<std::string>>(value);
            if (!res.empty() && *res.begin() == "zoom" && context.zoomLevel) {
                return *context.zoomLevel;
            }

            return value;
        } else {

            return value;
        }

    };
private:
    const ValueVariant value;
};

class HasPropertyValue : public Value {
public:
    HasPropertyValue(const std::string key) : key(key) {};

    std::unordered_set<std::string> getUsedKeys() override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        return context.feature.contains(key);
    };
private:
    const std::string key;
};


class ScaleValue : public Value {
public:
    ScaleValue(const std::shared_ptr<Value> value, const double scale) : value(value), scale(scale) {};

    std::unordered_set<std::string> getUsedKeys() override { return value->getUsedKeys(); }

    ValueVariant evaluate(const EvaluationContext &context) override {
        return std::visit(overloaded {
            [](std::string val){
                return 0.0;
            },
            [&](double val){
                return val * scale;
            },
            [&](int64_t val){
                return (double)val * scale;
            },
            [](bool val){
                return 0.0;
            },
            [](Color val){
                return 0.0;
            },
            [](std::vector<float> val){
                return 0.0;
            },
            [](std::vector<std::string> val){
                return 0.0;
            },
            [](std::vector<FormattedStringEntry> val){
                return 0.0;
            }
        }, value->evaluate(context));
    };
private:
    const std::shared_ptr<Value> value;
    const double scale;
};


class ExponentialInterpolation {
public:
    static double interpolationFactor(const double &base, const double &x, const double &a, const double &b) {
        double range = b - a;
        double progress = std::max(0.0, x - a);
        return (base == 1.0) ? (progress / range) : (std::pow(base, progress) - 1) / (std::pow(base, range) - 1);
    }
};

class InterpolatedValue : public Value {
public:
    InterpolatedValue(double interpolationBase, const std::vector<std::tuple<double, std::shared_ptr<Value>>> steps)
    : interpolationBase(interpolationBase), steps(steps) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &step: steps) {
            auto const setKeys = std::get<1>(step)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        double zoom = context.zoomLevel.has_value() ? context.zoomLevel.value() : 0.0;
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = std::get<0>(nextStep);
            if (nS >= zoom) {
                const auto &prevStep = steps[i];
                double pS = std::get<0>(prevStep);
                ValueVariant pV = std::get<1>(prevStep)->evaluate(context);
                ValueVariant nV = std::get<1>(nextStep)->evaluate(context);
                return interpolate(ExponentialInterpolation::interpolationFactor(interpolationBase, zoom, pS, nS), pV, nV);
            }
        }
        const auto last = std::get<1>(steps[maxStepInd]);
        return last->evaluate(context);
    }

    ValueVariant interpolate(const double &interpolationFactor, const ValueVariant &yBase, const ValueVariant &yTop) {

        if (std::holds_alternative<int64_t>(yBase) && std::holds_alternative<int64_t>(yTop)) {
            return std::get<int64_t>(yBase) + (std::get<int64_t>(yTop) - std::get<int64_t>(yBase)) * interpolationFactor;
        }

        if (std::holds_alternative<int64_t>(yBase) && std::holds_alternative<double>(yTop)) {
            return std::get<int64_t>(yBase) + (std::get<double>(yTop) - std::get<int64_t>(yBase)) * interpolationFactor;
        }

        if (std::holds_alternative<double>(yBase) && std::holds_alternative<int64_t>(yTop)) {
            return std::get<double>(yBase) + (std::get<int64_t>(yTop) - std::get<double>(yBase)) * interpolationFactor;
        }

        if (std::holds_alternative<double>(yBase) && std::holds_alternative<double>(yTop)) {
            return std::get<double>(yBase) + (std::get<double>(yTop) - std::get<double>(yBase)) * interpolationFactor;
        }

        if (std::holds_alternative<std::vector<float>>(yBase) && std::holds_alternative<std::vector<float>>(yTop)) {
            auto const &base = std::get<std::vector<float>>(yBase);
            auto const &top = std::get<std::vector<float>>(yTop);

            assert(base.size() == top.size());
            std::vector<float> result(base.size(), 0.0);
            for (size_t i = 0; i != base.size(); i++) {
                result[i] = base[i] + (top[i] - base[i]) * interpolationFactor;
            }
            return result;
        }

        if (std::holds_alternative<Color>(yBase) && std::holds_alternative<Color>(yTop)) {
            Color yBaseC = std::get<Color>(yBase);
            Color yTopC = std::get<Color>(yTop);
            return Color(yBaseC.r + (yTopC.r - yBaseC.r) * interpolationFactor,
                         yBaseC.g + (yTopC.g - yBaseC.g) * interpolationFactor,
                         yBaseC.b + (yTopC.b - yBaseC.b) * interpolationFactor,
                         yBaseC.a + (yTopC.a - yBaseC.a) * interpolationFactor);
        }
        assert(false);
        return 0;
    }

private:
    double interpolationBase;
    std::vector<std::tuple<double, std::shared_ptr<Value>>> steps;
};

class BezierInterpolatedValue : public Value {
public:
    BezierInterpolatedValue(double x1, double y1, double x2, double y2, const std::vector<std::tuple<double, std::shared_ptr<Value>>> steps)
            : bezier(x1, y1, x2, y2), steps(steps) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &step: steps) {
            auto const setKeys = std::get<1>(step)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        double zoom = context.zoomLevel.has_value() ? context.zoomLevel.value() : 0.0;
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = std::get<0>(nextStep);
            if (nS >= zoom) {
                const auto &prevStep = steps[i];
                double pS = std::get<0>(prevStep);
                ValueVariant pV = std::get<1>(prevStep)->evaluate(context);
                ValueVariant nV = std::get<1>(nextStep)->evaluate(context);

                auto factor = bezier.solve(1.0 - (nS - zoom) / (nS - pS), 1e-6);
                return interpolate(factor, pV, nV);
            }
        }

        int index = (steps.size() > 0 && zoom <= double(std::get<0>(steps[0]))) ? 0 : maxStepInd;
        const auto step = std::get<1>(steps[index]);
        return step->evaluate(context);
    }
    ValueVariant interpolate(const double &interpolationFactor, const ValueVariant &yBase, const ValueVariant &yTop) {

        if (std::holds_alternative<int64_t>(yBase) && std::holds_alternative<int64_t>(yTop)) {
            return std::get<int64_t>(yBase) + (std::get<int64_t>(yTop) - std::get<int64_t>(yBase)) * interpolationFactor;
        }

        if (std::holds_alternative<double>(yBase) && std::holds_alternative<double>(yTop)) {
            return std::get<double>(yBase) + (std::get<double>(yTop) - std::get<double>(yBase)) * interpolationFactor;
        }

        if (std::holds_alternative<Color>(yBase) && std::holds_alternative<Color>(yTop)) {
            Color yBaseC = std::get<Color>(yBase);
            Color yTopC = std::get<Color>(yTop);
            return Color(yBaseC.r + (yTopC.r - yBaseC.r) * interpolationFactor,
                         yBaseC.g + (yTopC.g - yBaseC.g) * interpolationFactor,
                         yBaseC.b + (yTopC.b - yBaseC.b) * interpolationFactor,
                         yBaseC.a + (yTopC.a - yBaseC.a) * interpolationFactor);
        }
        assert(false);
        return 0;
    }


private:
    UnitBezier bezier;
    std::vector<std::tuple<double, std::shared_ptr<Value>>> steps;
};

class StopValue : public Value {
public:
    StopValue(const std::vector<std::tuple<double, std::shared_ptr<Value>>> stops) : stops(stops) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &stop: stops) {
            auto const setKeys = std::get<1>(stop)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        double zoom = context.zoomLevel.has_value() ? context.zoomLevel.value() : 0.0;
        for (const auto &stop : stops) {
            double stopZoom = std::get<0>(stop);
            if (stopZoom > zoom) {
                return std::get<1>(stop)->evaluate(context);
            }
        }
        const auto last = std::get<1>(stops[(int) stops.size() - 1]);
        return last->evaluate(context);
    }
private:
    std::vector<std::tuple<double, std::shared_ptr<Value>>> stops;
};

enum class PropertyCompareType {
    EQUAL,
    NOTEQUAL,
    LESS,
    LESSEQUAL,
    GREATER,
    GREATEREQUAL
};

class ValueVariantCompareHelper {
public:
    static bool compare(const ValueVariant &lhs, const ValueVariant &rhs, PropertyCompareType type) {

        auto fallback = [](const ValueVariant &lhs, const ValueVariant &rhs, PropertyCompareType type) -> bool{
            switch (type) {
                case PropertyCompareType::EQUAL:
                    return lhs == rhs;
                case PropertyCompareType::NOTEQUAL:
                    return lhs != rhs;
                case PropertyCompareType::LESS:
                    return lhs < rhs;
                case PropertyCompareType::LESSEQUAL:
                    return lhs <= rhs;
                case PropertyCompareType::GREATER:
                    return lhs > rhs;
                case PropertyCompareType::GREATEREQUAL:
                    return lhs >= rhs;
            }
        };

        return std::visit(overloaded {
            [&](std::string lhsValue){
                return std::visit(overloaded {
                    [&lhsValue, &type](std::string rhsValue){
                        switch (type) {
                            case PropertyCompareType::EQUAL:
                                return lhsValue == rhsValue;
                            case PropertyCompareType::NOTEQUAL:
                                return lhsValue != rhsValue;
                            case PropertyCompareType::LESS:
                                return lhsValue < rhsValue;
                            case PropertyCompareType::LESSEQUAL:
                                return lhsValue <= rhsValue;
                            case PropertyCompareType::GREATER:
                                return lhsValue > rhsValue;
                            case PropertyCompareType::GREATEREQUAL:
                                return lhsValue >= rhsValue;
                        }
                    },
                    [&](double rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](int64_t rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](bool rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](Color rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<float> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<std::string> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<FormattedStringEntry> rhsValue){
                        return fallback(lhs, rhs, type);
                    }
                }, rhs);
            },
            [&](double lhsValue){
                return std::visit(overloaded {
                    [&](std::string rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&lhsValue, &type](double rhsValue){
                        switch (type) {
                            case PropertyCompareType::EQUAL:
                                return lhsValue == rhsValue;
                            case PropertyCompareType::NOTEQUAL:
                                return lhsValue != rhsValue;
                            case PropertyCompareType::LESS:
                                return lhsValue < rhsValue;
                            case PropertyCompareType::LESSEQUAL:
                                return lhsValue <= rhsValue;
                            case PropertyCompareType::GREATER:
                                return lhsValue > rhsValue;
                            case PropertyCompareType::GREATEREQUAL:
                                return lhsValue >= rhsValue;
                        }
                    },
                    [&lhsValue, &type](int64_t rhsValue){
                        double rhsDoubleValue = rhsValue;
                        switch (type) {
                            case PropertyCompareType::EQUAL:
                                return lhsValue == rhsDoubleValue;
                            case PropertyCompareType::NOTEQUAL:
                                return lhsValue != rhsDoubleValue;
                            case PropertyCompareType::LESS:
                                return lhsValue < rhsDoubleValue;
                            case PropertyCompareType::LESSEQUAL:
                                return lhsValue <= rhsDoubleValue;
                            case PropertyCompareType::GREATER:
                                return lhsValue > rhsDoubleValue;
                            case PropertyCompareType::GREATEREQUAL:
                                return lhsValue >= rhsDoubleValue;
                        }
                    },
                    [=](bool rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [=](Color rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [=](std::vector<float> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [=](std::vector<std::string> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [=](std::vector<FormattedStringEntry> rhsValue){
                        return fallback(lhs, rhs, type);
                    }
                }, rhs);
            },
            [&](int64_t lhsValue){
                return std::visit(overloaded {
                    [&](std::string rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&lhsValue, &type](double rhsValue){
                        int64_t rhsIntValue = rhsValue;
                        switch (type) {
                            case PropertyCompareType::EQUAL:
                                return lhsValue == rhsIntValue;
                            case PropertyCompareType::NOTEQUAL:
                                return lhsValue != rhsIntValue;
                            case PropertyCompareType::LESS:
                                return lhsValue < rhsIntValue;
                            case PropertyCompareType::LESSEQUAL:
                                return lhsValue <= rhsIntValue;
                            case PropertyCompareType::GREATER:
                                return lhsValue > rhsIntValue;
                            case PropertyCompareType::GREATEREQUAL:
                                return lhsValue >= rhsIntValue;
                        }
                    },
                    [&lhsValue, &type](int64_t rhsValue){
                        switch (type) {
                            case PropertyCompareType::EQUAL:
                                return lhsValue == rhsValue;
                            case PropertyCompareType::NOTEQUAL:
                                return lhsValue != rhsValue;
                            case PropertyCompareType::LESS:
                                return lhsValue < rhsValue;
                            case PropertyCompareType::LESSEQUAL:
                                return lhsValue <= rhsValue;
                            case PropertyCompareType::GREATER:
                                return lhsValue > rhsValue;
                            case PropertyCompareType::GREATEREQUAL:
                                return lhsValue >= rhsValue;
                        }
                    },
                    [&](bool rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](Color rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<float> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<std::string> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<FormattedStringEntry> rhsValue){
                        return fallback(lhs, rhs, type);
                    }
                }, rhs);
            },
            [&](bool lhsValue){
                return std::visit(overloaded {
                    [&](std::string rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](double rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](int64_t rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&lhsValue, &type](bool rhsValue){
                        switch (type) {
                            case PropertyCompareType::EQUAL:
                                return lhsValue == rhsValue;
                            case PropertyCompareType::NOTEQUAL:
                                return lhsValue != rhsValue;
                            case PropertyCompareType::LESS:
                                return lhsValue < rhsValue;
                            case PropertyCompareType::LESSEQUAL:
                                return lhsValue <= rhsValue;
                            case PropertyCompareType::GREATER:
                                return lhsValue > rhsValue;
                            case PropertyCompareType::GREATEREQUAL:
                                return lhsValue >= rhsValue;
                        }
                    },
                    [&](Color rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<float> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<std::string> rhsValue){
                        return fallback(lhs, rhs, type);
                    },
                    [&](std::vector<FormattedStringEntry> rhsValue){
                        return fallback(lhs, rhs, type);
                    }
                }, rhs);
            },
            [&](Color lhsValue){
                return fallback(lhs, rhs, type);
            },
            [&](std::vector<float> lhsValue){
                return fallback(lhs, rhs, type);
            },
            [&](std::vector<std::string> lhsValue){
                return fallback(lhs, rhs, type);
            },
            [&](std::vector<FormattedStringEntry> lhsValue){
                return fallback(lhs, rhs, type);
            }
        }, lhs);
    }
};

class StepValue : public Value {
public:
    StepValue(const std::shared_ptr<Value> compareValue, const std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops, std::shared_ptr<Value> defaultValue) : compareValue(compareValue), stops(stops), defaultValue(defaultValue) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &stop: stops) {
            auto const setKeys = std::get<1>(stop)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        auto compareValue_ = compareValue->evaluate(context);

        for (const auto &[stop, value] : stops) {
            if (ValueVariantCompareHelper::compare(stop->evaluate(context), compareValue_, PropertyCompareType::GREATER)) {
                return value->evaluate(context);
            }
        }
        return defaultValue->evaluate(context);
    }
private:
    const std::shared_ptr<Value> compareValue;
    std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops;
    std::shared_ptr<Value> defaultValue;
};

class CaseValue : public Value {
public:
    CaseValue(const std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases, std::shared_ptr<Value> defaultValue) : cases(cases), defaultValue(defaultValue) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &[condition, value]: cases) {
            if (condition) {
                auto const conditionKeys = condition->getUsedKeys();
                usedKeys.insert(conditionKeys.begin(), conditionKeys.end());
            }

            auto const valueKeys = value->getUsedKeys();
            usedKeys.insert(valueKeys.begin(), valueKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        for (auto const &[condition, value]: cases) {
            if (condition && condition->evaluateOr(context, false)) {
                return value->evaluate(context);
            }
        }
        return defaultValue->evaluate(context);
    }
private:
    const std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases;
    const std::shared_ptr<Value> defaultValue;
};


class ToNumberValue: public Value {
    const std::shared_ptr<Value> value;

public:
    ToNumberValue(const std::shared_ptr<Value> value): value(value) {}

    std::unordered_set<std::string> getUsedKeys() override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        return std::visit(overloaded {
            [](std::string val){
                return std::stod(val);
            },
            [](double val){
                return val;
            },
            [](int64_t val){
                return (double)val;
            },
            [](bool val){
                return 0.0;
            },
            [](Color val){
                return 0.0;
            },
            [](std::vector<float> val){
                return 0.0;
            },
            [](std::vector<std::string> val){
                return 0.0;
            },
            [](std::vector<FormattedStringEntry> val){
                return 0.0;
            }
        }, value->evaluate(context));
    };
};


class MatchValue: public Value {
    const std::shared_ptr<Value> compareValue;
    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> valueMapping;
    const std::shared_ptr<Value> defaultValue;

public:
    MatchValue(const std::shared_ptr<Value> compareValue,
               const std::map<std::set<ValueVariant>, std::shared_ptr<Value>> mapping,
               const std::shared_ptr<Value> defaultValue): compareValue(compareValue), defaultValue(defaultValue) {
        for (auto const &[keys, value]: mapping) {
            for(auto const& key : keys) {
                valueMapping.emplace_back(std::make_pair(key, value));
            }
        }
    }

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto const compareValueKeys = compareValue->getUsedKeys();
        usedKeys.insert(compareValueKeys.begin(), compareValueKeys.end());

        auto const defaultValueKeys = defaultValue->getUsedKeys();
        usedKeys.insert(defaultValueKeys.begin(), defaultValueKeys.end());

        for (auto const &i: valueMapping) {
            auto const valueKeys = i.second->getUsedKeys();
            usedKeys.insert(valueKeys.begin(), valueKeys.end());
        }

        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        const auto &value = compareValue->evaluate(context);

        for(const auto& i : valueMapping) {
            if(i.first == value) {
                return i.second->evaluate(context);
            }
        }

        return defaultValue->evaluate(context);
    };
};



class PropertyFilter : public Value {
private:
    std::shared_ptr<Value> defaultValue;
    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> valueMapping;
    const std::string key;

public:
    PropertyFilter(const std::map<std::set<ValueVariant>, std::shared_ptr<Value>> mapping,
                const std::shared_ptr<Value> defaultValue, const std::string &key): defaultValue(defaultValue), key(key) {
        for (auto const &entry: mapping) {
            for(auto const& v : entry.first) {
                valueMapping.emplace_back(std::make_pair(v, entry.second));
            }
        }
    }

    std::unordered_set<std::string> getUsedKeys() override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        const auto &p = context.feature.getValue(key);

        if (p) {
            for(const auto& m : valueMapping) {
                if(m.first == *p && m.second) {
                    return m.second->evaluate(context);
                }
            }
        }

        return defaultValue->evaluate(context);
    };
};



enum class LogOpType {
    AND,
    OR,
    NOT
};

class LogOpValue : public Value {
public:
    LogOpValue(const LogOpType &logOpType, const std::shared_ptr<Value> &lhs, const std::shared_ptr<Value> &rhs = nullptr) : logOpType(logOpType), lhs(lhs), rhs(rhs) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.insert(lhsKeys.begin(), lhsKeys.end());

        if (rhs) {
            auto const rhsKeys = rhs->getUsedKeys();
            usedKeys.insert(rhsKeys.begin(), rhsKeys.end());
        }

        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        switch (logOpType) {
            case LogOpType::AND:
                return lhs->evaluateOr(context, false) && rhs && rhs->evaluateOr(context, false);
            case LogOpType::OR:
                return lhs->evaluateOr(context, false) || (rhs && rhs->evaluateOr(context, false));
            case LogOpType::NOT:
                return !lhs->evaluateOr(context, false);
        }
    };

private:
    const LogOpType logOpType;
    const std::shared_ptr<Value> lhs;
    const std::shared_ptr<Value> rhs;
};

class AllValue: public Value {
public:
    AllValue(const std::vector<const std::shared_ptr<Value>> values) : values(values) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &value: values) {
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        for (auto const value: values) {
            if (!value->evaluateOr(context, false)) {
                return false;
            }
        }
        return true;
    };

private:
    const std::vector<const std::shared_ptr<Value>> values;
};

class AnyValue: public Value {
public:
    AnyValue(const std::vector<const std::shared_ptr<Value>> values) : values(values) {}

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &value: values) {
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        for (auto const value: values) {
            if (value->evaluateOr(context, false)) {
                return true;
            }
        }
        return false;
    };

private:
    const std::vector<const std::shared_ptr<Value>> values;
};

class PropertyCompareValue: public Value {
public:
    PropertyCompareValue(const std::shared_ptr<Value> lhs, const std::shared_ptr<Value> rhs, const PropertyCompareType type) : lhs(lhs), rhs( rhs), type(type) {
        assert(lhs);
        assert(rhs);
    }

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.insert(lhsKeys.begin(), lhsKeys.end());

        auto const rhsKeys = rhs->getUsedKeys();
        usedKeys.insert(rhsKeys.begin(), rhsKeys.end());

        return usedKeys;
    }

 ValueVariant evaluate(const EvaluationContext &context) override {
     auto const lhsValue = lhs->evaluate(context);
     auto const rhsValue = rhs->evaluate(context);

     return ValueVariantCompareHelper::compare(lhsValue, rhsValue, type);
 };

private:
    const std::shared_ptr<Value> lhs;
    const std::shared_ptr<Value> rhs;
    const PropertyCompareType type;
};

class InFilter : public Value {
private:
    const std::unordered_set<ValueVariant> values;
    const std::string key;
public:
    InFilter(const std::string &key, const std::unordered_set<ValueVariant> values) :values(values), key(key) {}

    std::unordered_set<std::string> getUsedKeys() override {
        return { key };
    }

 ValueVariant evaluate(const EvaluationContext &context) override {
        auto const value = context.feature.getValue(key);
        if (!value) {
            return false;
        }
        return values.count(*value) != 0;
    };
};

class NotInFilter : public Value {
private:
    const std::unordered_set<ValueVariant> values;
    const std::string key;
public:
    NotInFilter(const std::string &key, const std::unordered_set<ValueVariant> values) :values(values), key(key) {}

    std::unordered_set<std::string> getUsedKeys() override {
        return { key };
    }

 ValueVariant evaluate(const EvaluationContext &context) override {
        auto const value = context.feature.getValue(key);
        if (!value) {
            return true;
        }
        return values.count(*value) == 0;
    };
};

struct FormatValueWrapper {
    std::shared_ptr<Value> value;
    float scale;
};

class FormatValue : public Value {
public:
    FormatValue(const std::vector<FormatValueWrapper> values) : values(values) {};

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &wrapper: values) {
            auto const setKeys = wrapper.value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        std::vector<FormattedStringEntry> result;
        for (auto const &wrapper: values) {
            auto const evaluatedValue = ToStringValue(wrapper.value).evaluateOr(context, std::string());
            result.push_back({evaluatedValue, wrapper.scale});
        }
        return result;
    };
private:
    const std::vector<FormatValueWrapper> values;
};


enum class MathOperation {
    MINUS,
    PLUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    POWER
};

class MathValue: public Value {
public:
    MathValue(const std::shared_ptr<Value> lhs, const std::shared_ptr<Value> rhs, const MathOperation operation) : lhs(lhs), rhs( rhs), operation(operation) {
        assert(lhs);
        assert(rhs);
    }

    std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.insert(lhsKeys.begin(), lhsKeys.end());

        auto const rhsKeys = rhs->getUsedKeys();
        usedKeys.insert(rhsKeys.begin(), rhsKeys.end());

        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        auto const lhsValue = lhs->evaluateOr(context, (int64_t) 0);
        auto const rhsValue = rhs->evaluateOr(context, (int64_t) 0);
        switch (operation) {
            case MathOperation::MINUS:
                return lhsValue - rhsValue;
            case MathOperation::PLUS:
                return lhsValue + rhsValue;
            case MathOperation::MULTIPLY:
                return lhsValue * rhsValue;
            case MathOperation::DIVIDE:
                return lhsValue / rhsValue;
            case MathOperation::MODULO:
                return lhsValue % rhsValue;
            case MathOperation::POWER:
                return std::pow(lhsValue,  rhsValue);
        }
    };

private:
    const std::shared_ptr<Value> lhs;
    const std::shared_ptr<Value> rhs;
    const MathOperation operation;
};

class LenghtValue: public Value {
    const std::shared_ptr<Value> value;

public:
    LenghtValue(const std::shared_ptr<Value> value): value(value) {}

    std::unordered_set<std::string> getUsedKeys() override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) override {
        return std::visit(overloaded {
            [](std::string val){
                return (int64_t) val.size();
            },
            [](double val){
                return (int64_t) 0;
            },
            [](int64_t val){
                return (int64_t) 0;
            },
            [](bool val){
                return (int64_t) 0;
            },
            [](Color val){
                return (int64_t) 0;
            },
            [](std::vector<float> val){
                return (int64_t) val.size();
            },
            [](std::vector<std::string> val){
                return (int64_t) val.size();
            },
            [](std::vector<FormattedStringEntry> val){
                return (int64_t) val.size();
            }
        }, value->evaluate(context));
    };
};
