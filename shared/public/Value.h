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
#include "TextJustify.h"
#include "FormattedStringEntry.h"
#include "LineCapType.h"
#include "TextTransform.h"
#include "TextSymbolPlacement.h"
#include <sstream>
#include "ColorUtil.h"
#include <string>
#include "VectorLayerFeatureInfo.h"
#include "SymbolAlignment.h"
#include "IconTextFit.h"
#include "BlendMode.h"
#include "SymbolZOrder.h"

namespace std {
    template <>
    struct hash<Color> {
        size_t operator()(const Color &c) const {
            return std::hash<std::tuple<float, float, float, float>>()({c.r, c.g, c.b, c.a});
        }
    };

    template <>
    struct hash<std::vector<float>> {
        size_t operator()(const std::vector<float> &vector) const {
            size_t hash = 0;
            for(auto const &val: vector) {
                std::hash_combine(hash, std::hash<float>{}(val));
            }
            return hash;
        }
    };

    template <>
    struct hash<std::vector<std::string>> {
        size_t operator()(const std::vector<std::string> &vector) const {
            size_t hash = 0;
            for(auto const &val: vector) {
                std::hash_combine(hash, std::hash<std::string>{}(val));
            }
            return hash;
        }
    };

    template <>
    struct hash<std::vector<FormattedStringEntry>> {
        size_t operator()(const std::vector<FormattedStringEntry> &vector) const {
            size_t hash = 0;
            for(auto const &val: vector) {
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
                     std::vector<FormattedStringEntry>,
                     std::monostate> ValueVariant;

struct property_value_mapping : vtzero::property_value_mapping {
    using float_type = double;
    using uint_type = int64_t;
};

class FeatureContext {
public:
    using keyType = std::string;
    using valueType = ValueVariant;
    using mapType = std::vector<std::pair<keyType, valueType>>;
private:
    mapType propertiesMap;

public:
    uint64_t identifier;
    vtzero::GeomType geomType;

    FeatureContext() {}

    FeatureContext(const FeatureContext &other) :
    propertiesMap(std::move(other.propertiesMap)),
    geomType(other.geomType),
    identifier(other.identifier) {}


    FeatureContext(vtzero::GeomType geomType,
                   mapType propertiesMap,
                   uint64_t identifier):
    propertiesMap(std::move(propertiesMap)),
    geomType(geomType),
    identifier(identifier){
        initialize();
    }

    FeatureContext(vtzero::feature const &feature) {
        geomType = feature.geometry_type();

        feature.for_each_property([this] (const vtzero::property& p) {
            this->propertiesMap.push_back(std::make_pair(std::string(p.key()), vtzero::convert_property_value<ValueVariant, property_value_mapping>(p.value())));
            return true;
        });

        if (feature.has_id()) {
            identifier = feature.id();
        } else {
            size_t hash = 0;
            for(auto const &[key, val]: propertiesMap) {
                std::hash_combine(hash, std::hash<valueType>{}(val));
            }
            identifier = hash;
        }

        initialize();
    };

    void initialize() {
        propertiesMap.push_back(std::make_pair("identifier", int64_t(identifier)));

        switch (geomType) {
            case vtzero::GeomType::LINESTRING: {
                propertiesMap.push_back(std::make_pair("$type", "LineString"));
                break;
            }
            case vtzero::GeomType::POINT: {
                propertiesMap.push_back(std::make_pair("$type", "Point"));
                break;
            }
            case vtzero::GeomType::POLYGON: {
                propertiesMap.push_back(std::make_pair("$type", "Polygon"));
                break;
            }
            case vtzero::GeomType::UNKNOWN: {
                propertiesMap.push_back(std::make_pair("$type", "Unknown"));
                break;
            }
        }
    }

    bool contains(const std::string &key) const {
        for(const auto& p : propertiesMap) {
            if(p.first == key) {
                return true;
            }
        }

        return false;
    }

    ValueVariant getValue(const std::string &key) const {
        for(const auto& p : propertiesMap) {
            if(p.first == key) {
                return std::move(p.second);
            }
        }

        return std::monostate();
    }

    size_t getStyleHash(const std::unordered_set<std::string> &usedKeys) const {
        size_t hash = 0;
        for(auto const &[key, val]: propertiesMap) {
            if (usedKeys.count(key) != 0) {
                std::hash_combine(hash, std::hash<valueType>{}(val));
            }
        }
        return hash;
    }

    VectorLayerFeatureInfo getFeatureInfo() const {
        std::string identifier = std::to_string(this->identifier);
        std::unordered_map<std::string, VectorLayerFeatureInfoValue> properties;
        for(const auto &[key, val]: propertiesMap) {
            properties.insert({
                key,
                std::visit(overloaded {
                    [](const std::string &val){
                        return VectorLayerFeatureInfoValue(val, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
                    },
                    [](double val){
                        return VectorLayerFeatureInfoValue(std::nullopt, val, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
                    },
                    [](int64_t val){
                        return VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, val, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
                    },
                    [](bool val){
                        return VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, val, std::nullopt, std::nullopt, std::nullopt);
                    },
                    [](const Color &val){
                        return VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt, val, std::nullopt, std::nullopt);
                    },
                    [](const std::vector<float> &val){
                        return VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, val, std::nullopt);
                    },
                    [](const std::vector<std::string> &val){
                        return VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,  std::nullopt, val);
                    },
                    [](const std::vector<FormattedStringEntry> &val){
                        std::vector<std::string> strings;
                        for (const auto &string: val) {
                            strings.push_back(string.text);
                        }
                        return VectorLayerFeatureInfoValue( std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt,  strings);
                    },
                    [](const std::monostate &val) {
                        return VectorLayerFeatureInfoValue( std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt,  std::nullopt);
                    }
                }, val)
            });
        }
        return VectorLayerFeatureInfo(identifier, properties);
    }

    bool operator==(const FeatureContext &o) const { return propertiesMap == o.propertiesMap; }
};


class EvaluationContext {
public:
    std::optional<double> zoomLevel;
    const std::shared_ptr<FeatureContext> feature;

    EvaluationContext(std::optional<double> zoomLevel,
                      const std::shared_ptr<FeatureContext> feature) : zoomLevel(zoomLevel), feature(feature) {};
};

class Value {
public:
    Value() {};
    virtual ~Value() = default;

    virtual std::unique_ptr<Value> clone() = 0;

    virtual std::unordered_set<std::string> getUsedKeys() const { return {}; };

    virtual ValueVariant evaluate(const EvaluationContext &context) const = 0;

    virtual bool isEqual(const std::shared_ptr<Value> &other) const = 0;

    template<typename T>
    T evaluateOr(const EvaluationContext &context, const T &alternative) const {
        auto const &value = evaluate(context);
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }
        
        if constexpr (std::is_same<T, std::string>::value) {
            if (std::holds_alternative<std::vector<FormattedStringEntry>>(value)) {
                std::string concatenated = "";
                for (auto &s: std::get<std::vector<FormattedStringEntry>>(value)) {
                    concatenated += s.text;
                }
                return concatenated;
            }
        }

        // if a color is requested and we got a string we try to convert the string to a color
        if constexpr (std::is_same<T, Color>::value) {
            if (std::holds_alternative<std::string>(value)) {
                if (auto color = ColorUtil::fromString(std::get<std::string>(value))) {
                    return *color;
                }
            }
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
    Vec2F evaluateOr(const EvaluationContext &context, const Vec2F &alternative) const {
        auto const &value = evaluateOr(context, std::vector<float>{ alternative.x, alternative.y });
        return Vec2F(value[0], value[1]);
    }

    std::optional<::Anchor> anchorFromString(const std::string &value) const {
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

    std::optional<::BlendMode> blendModeFromString(const std::string &value) const {
        if (value == "multiply") {
            return BlendMode::MULTIPLY;
        } else  if (value == "normal") {
            return BlendMode::NORMAL;
        }
        return std::nullopt;
    }

    std::optional<::SymbolZOrder> symbolZOrderFromString(const std::string &value) const {
        if (value == "source") {
            return SymbolZOrder::SOURCE;
        } else if (value == "viewport-y") {
            return SymbolZOrder::VIEWPORT_Y;
        } else if (value == "auto") {
            return SymbolZOrder::AUTO;
        }
        return std::nullopt;
    }


    std::optional<::SymbolAlignment> alignmentFromString(const std::string &value) const {
        if (value == "auto") {
            return SymbolAlignment::AUTO;
        } else if (value == "map") {
            return SymbolAlignment::MAP;
        } else if (value == "viewport") {
            return SymbolAlignment::VIEWPORT;
        }
        return std::nullopt;
    }

    std::optional<::IconTextFit> iconTextFitFromString(const std::string &value) const {
        if (value == "none") {
            return IconTextFit::NONE;
        } else if (value == "width") {
            return IconTextFit::WIDTH;
        } else if (value == "height") {
            return IconTextFit::HEIGHT;
        } else if (value == "both") {
            return IconTextFit::BOTH;
        }
        return std::nullopt;
    }


    std::optional<::TextJustify> justifyFromString(const std::string &value) const {
        if (value == "auto") {
            return TextJustify::AUTO;
        } else if (value == "center") {
            return TextJustify::CENTER;
        } else if (value == "left") {
            return TextJustify::LEFT;
        } else if (value == "right") {
            return TextJustify::RIGHT;
        }
        return std::nullopt;
    }

    std::optional<::TextSymbolPlacement> textSymbolPlacementFromString(const std::string &value) const {
        if(value == "point") { return TextSymbolPlacement::POINT; }
        if(value == "line") { return TextSymbolPlacement::LINE; }
        if(value == "line-center") { return TextSymbolPlacement::LINE_CENTER; }
        return std::nullopt;
    }

    template<>
    BlendMode evaluateOr(const EvaluationContext &context, const BlendMode &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto blendMode = blendModeFromString(value);
        if (blendMode) {
            return *blendMode;
        }
        return alternative;
    }

    template<>
    SymbolZOrder evaluateOr(const EvaluationContext &context, const SymbolZOrder &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto symbolZOrder = symbolZOrderFromString(value);
        if (symbolZOrder) {
            return *symbolZOrder;
        }
        return alternative;
    }

    template<>
    Anchor evaluateOr(const EvaluationContext &context, const Anchor &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto anchor = anchorFromString(value);
        if (anchor) {
            return *anchor;
        }
        return alternative;
    }

    template<>
    SymbolAlignment evaluateOr(const EvaluationContext &context, const SymbolAlignment &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto alignment = alignmentFromString(value);
        if (alignment) {
            return *alignment;
        }
        return alternative;
    }

    template<>
    IconTextFit evaluateOr(const EvaluationContext &context, const IconTextFit &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto textFit = iconTextFitFromString(value);
        if (textFit) {
            return *textFit;
        }
        return alternative;
    }

    template<>
    TextJustify evaluateOr(const EvaluationContext &context, const TextJustify &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto anchor = justifyFromString(value);
        if (anchor) {
            return *anchor;
        }
        return alternative;
    }

    template<>
    TextSymbolPlacement evaluateOr(const EvaluationContext &context, const TextSymbolPlacement &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto placement = textSymbolPlacementFromString(value);
        if (placement) {
            return *placement;
        }
        return alternative;
    }

    std::optional<LineCapType> capTypeFromString(const std::string &value) const {
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
    LineCapType evaluateOr(const EvaluationContext &context, const LineCapType &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto type = capTypeFromString(value);
        if (type) {
            return *type;
        }
        return alternative;
    }

    std::optional<TextTransform> textTransformFromString(const std::string &value) const {
        if (value == "none") {
            return TextTransform::NONE;
        } else if (value == "uppercase") {
            return TextTransform::UPPERCASE;
        }
        return std::nullopt;
    }

    template<>
    TextTransform evaluateOr(const EvaluationContext &context, const TextTransform &alternative) const {
        auto const &value = evaluateOr(context, std::string(""));
        auto type = textTransformFromString(value);
        if (type) {
            return *type;
        }
        return alternative;
    }

    template<>
    std::vector<Anchor> evaluateOr(const EvaluationContext &context, const std::vector<Anchor> &alternative) const {
        auto const &values = evaluateOr(context, std::vector<std::string>());
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

template<class ResultType>
class ValueEvaluator {
public:
    ValueEvaluator() {}

    inline ResultType getResult(const std::shared_ptr<Value> &value, const EvaluationContext &context, const ResultType defaultValue) {
        if (!value) {
            return defaultValue;
        }
        if (lastValuePtr != value.get()) {
            lastResults.clear();
            staticValue = std::nullopt;
            const auto usedKeys = value->getUsedKeys();
            isStatic = usedKeys.empty();
            if (isStatic) {
                staticValue = value->evaluateOr(context, defaultValue);
            } else {
                isZoomdependent = usedKeys.count("zoom") != 0;
            }
            lastValuePtr = value.get();
        }

        if (isStatic) {
            return *staticValue;
        }

        const auto identifier = (context.feature->identifier << 12) | (uint64_t)((isZoomdependent ? (context.zoomLevel ? *context.zoomLevel : 0.f) : 0.f) * 100);

        const auto lastResultIt = lastResults.find(identifier);
        if (lastResultIt != lastResults.end()) {
            return lastResultIt->second;
        }

        const auto result = value->evaluateOr(context, defaultValue);
        lastResults.insert({identifier, result});
        return result;
    }

private:
    std::unordered_map<uint64_t, ResultType> lastResults;

    std::optional<ResultType> staticValue;

    bool isZoomdependent = false;
    bool isStatic = false;
    void* lastValuePtr = nullptr;
};

class GetPropertyValue : public Value {
public:
    GetPropertyValue(const std::string key) : key(key) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<GetPropertyValue>(key);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        if (key == "zoom" && context.zoomLevel) {
            return *context.zoomLevel;
        }

        const auto& result = context.feature->getValue(key);
        if(!std::holds_alternative<std::monostate>(result)) {
            return result;
        }

        return "";
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<GetPropertyValue>(other)) {
            return casted->key == key;
        }
        return false;
    };
private:
    const std::string key;
};

class ToStringValue: public Value {
    const std::shared_ptr<Value> value;

public:
    ToStringValue(const std::shared_ptr<Value> value): value(value) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<ToStringValue>(value->clone());
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return std::visit(overloaded {
            [](const std::string &val){
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
            [](const Color &val){
                return std::string("");
            },
            [](const std::vector<float> &val){
                return std::string("");
            },
            [](const std::vector<std::string> &val){
                return std::string("");
            },
            [](const std::vector<FormattedStringEntry> &val){
                std::string string = "";
                for(auto const v: val) {
                    string += v.text;
                }
                return string;
            },
            [](const std::monostate &val) {
                return std::string("");
            }
        }, value->evaluate(context));
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<ToStringValue>(other)) {
            return value && casted->value && casted->value->isEqual(value);
        }
        return false;
    };
};

class StaticValue : public Value {
public:
    StaticValue(const ValueVariant value) : value(value) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<StaticValue>(value);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        if (std::holds_alternative<std::string>(value)) {
            std::string res = std::get<std::string>(value);
            std::unordered_set<std::string> usedKeys = { res };

            auto begin = res.find("{");
            auto end = res.find("}", begin);

            while ( begin != std::string::npos &&
                    end != std::string::npos &&
                    end > begin &&
                    (begin == 0 || res[begin - 1] != '\\') &&
                    (end == 0 || res[end - 1] != '\\')) {

                std::string key = res.substr (begin + 1,(end - begin) - 1);
                usedKeys.insert(key);
                begin = res.find("{", (begin + key.size()));
                end = res.find("}", begin);
            }

            return usedKeys;

        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            std::vector<std::string> res = std::get<std::vector<std::string>>(value);
            if (!res.empty() && *res.begin() == "zoom") {
                return { "zoom" };
            }
            return {};
        } else {
            return {};
        }

    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        if (std::holds_alternative<std::string>(value)) {
            std::string res = std::get<std::string>(value);

            const auto &result = context.feature->getValue(res);

            if(!std::holds_alternative<std::monostate>(result)) {
                return result;
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

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<StaticValue>(other)) {
            return casted->value == value;
        }
        return false;
    };
private:
    const ValueVariant value;
};

class HasPropertyValue : public Value {
public:
    HasPropertyValue(const std::string key) : key(key) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<HasPropertyValue>(key);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return context.feature->contains(key);
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<HasPropertyValue>(other)) {
            return casted->key == key;
        }
        return false;
    };
private:
    const std::string key;
};


class ScaleValue : public Value {
public:
    ScaleValue(const std::shared_ptr<Value> value, const double scale) : value(value), scale(scale) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<ScaleValue>(value->clone(), scale);
    }

    std::unordered_set<std::string> getUsedKeys() const override { return value->getUsedKeys(); }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return std::visit(overloaded {
            [](const std::string &val){
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
            [](const std::vector<float> &val){
                return 0.0;
            },
            [](const std::vector<std::string> &val){
                return 0.0;
            },
            [](const std::vector<FormattedStringEntry> &val){
                return 0.0;
            },
            [](const std::monostate &val){
                return 0.0;
            }
        }, value->evaluate(context));
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<ScaleValue>(other)) {
            return value && casted->value && casted->value->isEqual(value) && scale == casted->scale;
        }
        return false;
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
    InterpolatedValue(double interpolationBase, const std::vector<std::tuple<double, std::shared_ptr<Value>>> &steps)
    : interpolationBase(interpolationBase), steps(steps) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::tuple<double, std::shared_ptr<Value>>> clonedSteps;
        for (const auto &[step, value] : steps) {
            clonedSteps.emplace_back(step, value->clone());
        }
        return std::make_unique<InterpolatedValue>(interpolationBase, std::move(clonedSteps));
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys = { "zoom" };
        for (auto const &step: steps) {
            auto const setKeys = std::get<1>(step)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        double zoom = context.zoomLevel.has_value() ? context.zoomLevel.value() : 0.0;
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = std::get<0>(nextStep);
            if (nS >= zoom) {
                const auto &prevStep = steps[i];
                double pS = std::get<0>(prevStep);
                const ValueVariant &pV = std::get<1>(prevStep)->evaluate(context);
                const ValueVariant &nV = std::get<1>(nextStep)->evaluate(context);
                return interpolate(ExponentialInterpolation::interpolationFactor(interpolationBase, zoom, pS, nS), pV, nV);
            }
        }
        const auto last = std::get<1>(steps[maxStepInd]);
        return last->evaluate(context);
    }

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<InterpolatedValue>(other)) {
            if (casted->interpolationBase != interpolationBase) {
                return false;
            }
            if (casted->steps.size() != steps.size()) {
                return false;
            }
            for (int i = 0; i < steps.size(); i++) {
                if (std::get<0>(casted->steps[i]) != std::get<0>(steps[i])) {
                    return false;
                }
                if (std::get<1>(casted->steps[i]) && std::get<1>(steps[i]) && !std::get<1>(casted->steps[i])->isEqual(std::get<1>(steps[i]))) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    ValueVariant interpolate(const double &interpolationFactor, const ValueVariant &yBase, const ValueVariant &yTop) const {

        if (std::holds_alternative<std::string>(yBase) && std::holds_alternative<std::string>(yTop)) {
            if (interpolationFactor <  0.5 ){
                return yBase;
            } else {
                return yTop;
            }
        }

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
    BezierInterpolatedValue(double x1, double y1, double x2, double y2, const std::vector<std::tuple<double, std::shared_ptr<Value>>> &steps)
            : bezier(x1, y1, x2, y2), steps(steps) {}

    BezierInterpolatedValue(const UnitBezier &bezier, const std::vector<std::tuple<double, std::shared_ptr<Value>>> &steps)
            : bezier(bezier), steps(steps) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::tuple<double, std::shared_ptr<Value>>> clonedSteps;
        for (const auto &[step, value] : steps) {
            clonedSteps.emplace_back(step, value->clone());
        }
        return std::make_unique<BezierInterpolatedValue>(bezier, std::move(clonedSteps));
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys = {"zoom"};
        for (auto const &step: steps) {
            auto const setKeys = std::get<1>(step)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
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

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<BezierInterpolatedValue>(other)) {
            if (casted->bezier != bezier) {
                return false;
            }
            if (casted->steps.size() != steps.size()) {
                return false;
            }
            for (int i = 0; i < steps.size(); i++) {
                if (std::get<0>(casted->steps[i]) != std::get<0>(steps[i])) {
                    return false;
                }
                if (std::get<1>(casted->steps[i]) && std::get<1>(steps[i]) && !std::get<1>(casted->steps[i])->isEqual(std::get<1>(steps[i]))) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    ValueVariant interpolate(const double &interpolationFactor, const ValueVariant &yBase, const ValueVariant &yTop) const {

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
    const UnitBezier bezier;
    const std::vector<std::tuple<double, std::shared_ptr<Value>>> steps;
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

        if (std::holds_alternative<int64_t>(lhs) &&
            std::holds_alternative<double>(rhs)) {
            double lhsDouble = (double)std::get<int64_t>(lhs);
            double rhsDouble = std::get<double>(rhs);
            switch (type) {
                case PropertyCompareType::EQUAL:
                    return lhsDouble == rhsDouble;
                case PropertyCompareType::NOTEQUAL:
                    return lhsDouble != rhsDouble;
                case PropertyCompareType::LESS:
                    return lhsDouble < rhsDouble;
                case PropertyCompareType::LESSEQUAL:
                    return lhsDouble <= rhsDouble;
                case PropertyCompareType::GREATER:
                    return lhsDouble > rhsDouble;
                case PropertyCompareType::GREATEREQUAL:
                    return lhsDouble >= rhsDouble;
            }
        }

        if (std::holds_alternative<double>(lhs) &&
            std::holds_alternative<int64_t>(rhs)) {
            double lhsDouble = std::get<double>(lhs);
            double rhsDouble = (double) std::get<int64_t>(rhs);
            switch (type) {
                case PropertyCompareType::EQUAL:
                    return lhsDouble == rhsDouble;
                case PropertyCompareType::NOTEQUAL:
                    return lhsDouble != rhsDouble;
                case PropertyCompareType::LESS:
                    return lhsDouble < rhsDouble;
                case PropertyCompareType::LESSEQUAL:
                    return lhsDouble <= rhsDouble;
                case PropertyCompareType::GREATER:
                    return lhsDouble > rhsDouble;
                case PropertyCompareType::GREATEREQUAL:
                    return lhsDouble >= rhsDouble;
            }
        }

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

        return false;

    }
};

class StepValue : public Value {
public:
    StepValue(const std::shared_ptr<Value> compareValue,
              const std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops,
              std::shared_ptr<Value> defaultValue) : compareValue(compareValue), stops(stops), defaultValue(defaultValue) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> clonedStops;
        for (const auto &[v1, v2] : stops) {
            clonedStops.emplace_back(v1->clone(), v2->clone());
        }
        return std::make_unique<StepValue>(compareValue->clone(), std::move(clonedStops), defaultValue->clone());
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;
        
        auto const compareValueKeys = compareValue->getUsedKeys();
        usedKeys.insert(compareValueKeys.begin(), compareValueKeys.end());

        auto const defaultValueKeys = defaultValue->getUsedKeys();
        usedKeys.insert(defaultValueKeys.begin(), defaultValueKeys.end());

        for (auto const &stop: stops) {
            auto const setKeys = std::get<1>(stop)->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        const auto &compareValue_ = compareValue->evaluate(context);

        for (auto it = stops.begin(); it != stops.end(); it++) {
            if (ValueVariantCompareHelper::compare(std::get<0>(*it)->evaluate(context), compareValue_, PropertyCompareType::GREATER)) {
                if (it != stops.begin()) {
                    return std::get<1>(*std::prev(it))->evaluate(context);
                } else {
                    return defaultValue->evaluate(context);
                }
            }
        }
        return std::get<1>(*stops.rbegin())->evaluate(context);
    }
    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<StepValue>(other)) {
            // Compare the compareValue member
            if (!compareValue->isEqual(casted->compareValue)) {
                return false;
            }

            // Compare the stops member
            if (stops.size() != casted->stops.size()) {
                return false;
            }

            for (size_t i = 0; i < stops.size(); ++i) {
                const auto &thisStop = stops[i];
                const auto &otherStop = casted->stops[i];

                // Compare the first value in the tuple
                if (std::get<0>(thisStop) && std::get<0>(otherStop) && !std::get<0>(thisStop)->isEqual(std::get<0>(otherStop))) {
                    return false;
                }

                // Compare the second value in the tuple
                if (std::get<1>(thisStop) && std::get<1>(otherStop) && !std::get<1>(thisStop)->isEqual(std::get<1>(otherStop))) {
                    return false;
                }
            }

            // Compare the defaultValue member
            if (!defaultValue->isEqual(casted->defaultValue)) {
                return false;
            }

            return true; // All members are equal
        }

        return false; // Not the same type or nullptr
    }

private:
    const std::shared_ptr<Value> compareValue;
    std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops;
    std::shared_ptr<Value> defaultValue;
};

class CaseValue : public Value {
public:
    CaseValue(const std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases, std::shared_ptr<Value> defaultValue) : cases(cases), defaultValue(defaultValue) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> clonedCases;
        for (const auto &[v1, v2] : cases) {
            clonedCases.emplace_back(v1->clone(), v2->clone());
        }
        return std::make_unique<CaseValue>(std::move(clonedCases), defaultValue->clone());
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;

        auto const defaultValueKeys = defaultValue->getUsedKeys();
        usedKeys.insert(defaultValueKeys.begin(), defaultValueKeys.end());

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

    ValueVariant evaluate(const EvaluationContext &context) const override {
        for (auto const &[condition, value]: cases) {
            if (condition && condition->evaluateOr(context, false)) {
                return value->evaluate(context);
            }
        }
        return defaultValue->evaluate(context);
    }

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<CaseValue>(other)) {
            // Compare the cases member
            if (cases.size() != casted->cases.size()) {
                return false;
            }

            for (size_t i = 0; i < cases.size(); ++i) {
                const auto &thisCase = cases[i];
                const auto &otherCase = casted->cases[i];

                // Compare the first value in the tuple (condition)
                if (std::get<0>(thisCase) && std::get<0>(otherCase) && !std::get<0>(thisCase)->isEqual(std::get<0>(otherCase))) {
                    return false;
                }

                // Compare the second value in the tuple (value)
                if (!std::get<1>(thisCase)->isEqual(std::get<1>(otherCase))) {
                    return false;
                }
            }

            // Compare the defaultValue member
            if (!defaultValue->isEqual(casted->defaultValue)) {
                return false;
            }

            return true; // All members are equal
        }

        return false; // Not the same type or nullptr
    }

private:
    const std::vector<std::tuple<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases;
    const std::shared_ptr<Value> defaultValue;
};


class ToNumberValue: public Value {
    const std::shared_ptr<Value> value;

public:
    ToNumberValue(const std::shared_ptr<Value> value): value(value) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<ToNumberValue>(value->clone());
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return std::visit(overloaded {
            [](const std::string &val){
                try {
                    return std::stod(val);
                } catch (const std::invalid_argument&) {
                    return 0.0;
                } catch (const std::out_of_range&) {
                    return 0.0;
                }
            },
            [](double val){
                return val;
            },
            [](int64_t val){
                return (double)val;
            },
            [](bool val){
                return val ? 1.0 : 0.0;
            },
            [](const Color &val){
                return 0.0;
            },
            [](const std::vector<float> &val){
                return 0.0;
            },
            [](const std::vector<std::string> &val){
                return 0.0;
            },
            [](const std::vector<FormattedStringEntry> &val){
                return 0.0;
            },
            [](const std::monostate &val) {
                return 0.0;
            }
        }, value->evaluate(context));
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<ToNumberValue>(other)) {
            // Compare the value member
            if (value && casted->value && !value->isEqual(casted->value)) {
                return false;
            }
            return true; // All members are equal
        }
        return false; // Not the same type or nullptr
    }
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

    MatchValue(const std::shared_ptr<Value> compareValue,
               const std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> &mapping,
               const std::shared_ptr<Value> defaultValue)
            : compareValue(compareValue), valueMapping(mapping), defaultValue(defaultValue) {}


    std::unique_ptr<Value> clone() override {
        std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> clonedMapping;
        for (const auto &[variant, value] : valueMapping) {
            clonedMapping.emplace_back(variant, value->clone());
        }
        return std::make_unique<MatchValue>(compareValue->clone(), std::move(clonedMapping), defaultValue->clone());
    }

    std::unordered_set<std::string> getUsedKeys() const override {
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

    ValueVariant evaluate(const EvaluationContext &context) const override {
        const auto &value = compareValue->evaluate(context);

        for(const auto& i : valueMapping) {
            if(i.first == value) {
                return i.second->evaluate(context);
            }
        }

        return defaultValue->evaluate(context);
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<MatchValue>(other)) {
            // Compare the compareValue member
            if (!compareValue->isEqual(casted->compareValue)) {
                return false;
            }
            // Compare the valueMapping member
            if (valueMapping.size() != casted->valueMapping.size()) {
                return false;
            }

            for (size_t i = 0; i < valueMapping.size(); ++i) {
                const auto &thisPair = valueMapping[i];
                const auto &otherPair = casted->valueMapping[i];
                // Compare the first value in the pair (ValueVariant)
                if (thisPair.first != otherPair.first) {
                    return false;
                }
                // Compare the second value in the pair (Value)
                if (thisPair.second && otherPair.second && !thisPair.second->isEqual(otherPair.second)) {
                    return false;
                }
            }
            // Compare the defaultValue member
            if (!defaultValue->isEqual(casted->defaultValue)) {
                return false;
            }
            return true; // All members are equal
        }
        return false; // Not the same type or nullptr
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

    PropertyFilter(const std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> &mapping,
                const std::shared_ptr<Value> defaultValue, const std::string &key)
                : defaultValue(defaultValue), valueMapping(mapping), key(key) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::pair<ValueVariant , std::shared_ptr<Value>>> clonedMapping;
        for (const auto &[variant, value] : valueMapping) {
            clonedMapping.emplace_back(variant, value->clone());
        }
        return std::make_unique<PropertyFilter>(std::move(clonedMapping), defaultValue->clone(), key);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        const auto &p = context.feature->getValue(key);

        if(!std::holds_alternative<std::monostate>(p)) {
            for(const auto& m : valueMapping) {
                if(m.first == p && m.second) {
                    return m.second->evaluate(context);
                }
            }
        }

        return defaultValue->evaluate(context);
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<PropertyFilter>(other)) {
            // Compare the defaultValue member
            if (!defaultValue->isEqual(casted->defaultValue)) {
                return false;
            }

            // Compare the valueMapping member
            if (valueMapping.size() != casted->valueMapping.size()) {
                return false;
            }

            for (size_t i = 0; i < valueMapping.size(); ++i) {
                const auto &thisPair = valueMapping[i];
                const auto &otherPair = casted->valueMapping[i];

                // Compare the first value in the pair (ValueVariant)
                if (thisPair.first != otherPair.first) {
                    return false;
                }

                // Compare the second value in the pair (Value)
                if (thisPair.second && otherPair.second && !thisPair.second->isEqual(otherPair.second)) {
                    return false;
                }
            }

            // Compare the key member
            if (key != casted->key) {
                return false;
            }

            return true; // All members are equal
        }

        return false; // Not the same type or nullptr
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

    std::unique_ptr<Value> clone() override {
        switch (logOpType) {
            case LogOpType::AND:
            case LogOpType::OR:
                return std::make_unique<LogOpValue>(logOpType, lhs->clone(), rhs->clone());
            case LogOpType::NOT:
                return std::make_unique<LogOpValue>(logOpType, lhs->clone());
        }
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.insert(lhsKeys.begin(), lhsKeys.end());

        if (rhs) {
            auto const rhsKeys = rhs->getUsedKeys();
            usedKeys.insert(rhsKeys.begin(), rhsKeys.end());
        }

        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        switch (logOpType) {
            case LogOpType::AND:
                return lhs->evaluateOr(context, false) && rhs && rhs->evaluateOr(context, false);
            case LogOpType::OR:
                return lhs->evaluateOr(context, false) || (rhs && rhs->evaluateOr(context, false));
            case LogOpType::NOT:
                return !lhs->evaluateOr(context, false);
        }
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<LogOpValue>(other)) {
            // Compare the logOpType member
            if (logOpType != casted->logOpType) {
                return false;
            }

            if (lhs && casted->lhs && !lhs->isEqual(casted->lhs)) {
                return false;
            }

            if (rhs && casted->rhs && !rhs->isEqual(casted->rhs)) {
                return false;
            }

            return true; // All members are equal
        }

        return false; // Not the same type or nullptr
    };

private:
    const LogOpType logOpType;
    const std::shared_ptr<Value> lhs;
    const std::shared_ptr<Value> rhs;
};

class AllValue: public Value {
public:
    AllValue(const std::vector<const std::shared_ptr<Value>> values) : values(values) {}

    std::unique_ptr<Value> clone() override {
        std::vector<const std::shared_ptr<Value>> clonedValues;
        for (const auto &value : values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<AllValue>(std::move(clonedValues));
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &value: values) {
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        for (auto const value: values) {
            if (!value->evaluateOr(context, false)) {
                return false;
            }
        }
        return true;
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<AllValue>(other)) {
            // Compare the sizes of the values vectors
            if (values.size() != casted->values.size()) {
                return false;
            }

            // Compare each value in the values vectors
            for (size_t i = 0; i < values.size(); ++i) {
                if (values[i] && casted->values[i] && !values[i]->isEqual(casted->values[i])) {
                    return false;
                }
            }
            return true; // All values are equal
        }

        return false; // Not the same type or nullptr
    };


private:
    const std::vector<const std::shared_ptr<Value>> values;
};

class AnyValue: public Value {
public:
    AnyValue(const std::vector<const std::shared_ptr<Value>> values) : values(values) {}

    std::unique_ptr<Value> clone() override {
        std::vector<const std::shared_ptr<Value>> clonedValues;
        for (const auto &value : values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<AnyValue>(std::move(clonedValues));
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &value: values) {
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        for (auto const &value: values) {
            if (value->evaluateOr(context, false)) {
                return true;
            }
        }
        return false;
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<AnyValue>(other)) {
            // Compare the sizes of the values vectors
            if (values.size() != casted->values.size()) {
                return false;
            }
            
            // Compare each value in the values vectors
            for (size_t i = 0; i < values.size(); ++i) {
                if (values[i] && casted->values[i] && !values[i]->isEqual(casted->values[i])) {
                    return false;
                }
            }
            return true; // All values are equal
        }
        
        return false; // Not the same type or nullptr
    }

private:
    const std::vector<const std::shared_ptr<Value>> values;
};

class PropertyCompareValue: public Value {
public:
    PropertyCompareValue(const std::shared_ptr<Value> lhs, const std::shared_ptr<Value> rhs, const PropertyCompareType type) : lhs(lhs), rhs( rhs), type(type) {
        assert(lhs);
        assert(rhs);
    }

    std::unique_ptr<Value> clone() override {
        return std::make_unique<PropertyCompareValue>(lhs->clone(), rhs->clone(), type);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.insert(lhsKeys.begin(), lhsKeys.end());

        auto const rhsKeys = rhs->getUsedKeys();
        usedKeys.insert(rhsKeys.begin(), rhsKeys.end());

        return usedKeys;
    }

 ValueVariant evaluate(const EvaluationContext &context) const override {
     auto const &lhsValue = lhs->evaluate(context);
     auto const &rhsValue = rhs->evaluate(context);

     if (std::holds_alternative<Color>(lhsValue) && std::holds_alternative<std::string>(rhsValue)) {
         auto lhsColor = std::get<Color>(lhsValue);
         if (auto rhsColor = ColorUtil::fromString(std::get<std::string>(rhsValue))) {
             return lhsColor == *rhsColor;
         }
     }
     
     if (std::holds_alternative<std::string>(lhsValue) && std::holds_alternative<Color>(rhsValue)) {
         auto rhsColor = std::get<Color>(rhsValue);
         if (auto lhsColor = ColorUtil::fromString(std::get<std::string>(lhsValue))) {
             return *lhsColor == rhsColor;
         }
     }
     
     return ValueVariantCompareHelper::compare(lhsValue, rhsValue, type);
 };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<PropertyCompareValue>(other)) {
            if (lhs && casted->lhs && !lhs->isEqual(casted->lhs)) {
                return false;
            }

            if (rhs && casted->rhs && !rhs->isEqual(casted->rhs)) {
                return false;
            }

            // Compare the comparison type
            if (type != casted->type) {
                return false;
            }

            return true; // All values and types are equal
        }

        return false; // Not the same type or nullptr
    }

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

    std::unique_ptr<Value> clone() override {
        return std::make_unique<InFilter>(key, values);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        auto const &value = context.feature->getValue(key);
        return values.count(value) != 0;
    };


    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<InFilter>(other)) {
            // Compare the key
            if (key != casted->key) {
                return false;
            }

            // Compare the values
            if (values != casted->values) {
                return false;
            }

            return true; // Key and values are equal
        }

        return false; // Not the same type or nullptr
    }

};

class NotInFilter : public Value {
private:
    const std::unordered_set<ValueVariant> values;
    const std::string key;
public:
    NotInFilter(const std::string &key, const std::unordered_set<ValueVariant> values) :values(values), key(key) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<NotInFilter>(key, values);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return { key };
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        auto const &value = context.feature->getValue(key);
        return values.count(value) == 0;
    };


    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<NotInFilter>(other)) {
            // Compare the key
            if (key != casted->key) {
                return false;
            }
            // Compare the values
            if (values != casted->values) {
                return false;
            }
            return true; // Key and values are equal
        }
        return false; // Not the same type or nullptr
    }
};

struct FormatValueWrapper {
    FormatValueWrapper(const std::shared_ptr<Value> value, float scale) : value(value), scale(scale) {}
    std::shared_ptr<Value> value;
    float scale;
};

class FormatValue : public Value {
public:
    FormatValue(const std::vector<FormatValueWrapper> values) : values(values) {};

    std::unique_ptr<Value> clone() override {
        std::vector<FormatValueWrapper> clonedValues;
        for (const auto &value : values) {
            clonedValues.emplace_back(value.value->clone(), value.scale);
        }
        return std::make_unique<FormatValue>(std::move(values));
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;
        for (auto const &wrapper: values) {
            auto const setKeys = wrapper.value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        std::vector<FormattedStringEntry> result;
        for (auto const &wrapper: values) {
            auto const evaluatedValue = ToStringValue(wrapper.value).evaluateOr(context, std::string());
            result.push_back({evaluatedValue, wrapper.scale});
        }
        return result;
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<FormatValue>(other)) {
            // Compare the values
            if (values.size() != casted->values.size()) {
                return false;
            }

            for (size_t i = 0; i < values.size(); i++) {
                const FormatValueWrapper& thisWrapper = values[i];
                const FormatValueWrapper& otherWrapper = casted->values[i];

                if (thisWrapper.value && otherWrapper.value && !thisWrapper.value->isEqual(otherWrapper.value)) {
                    return false;
                }
                if (thisWrapper.scale != otherWrapper.scale) {
                    return false;
                }
            }

            return true; // Values are equal
        }

        return false; // Not the same type or nullptr
    }

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

    std::unique_ptr<Value> clone() override {
        return std::make_unique<MathValue>(lhs->clone(), rhs->clone(), operation);
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.insert(lhsKeys.begin(), lhsKeys.end());

        auto const rhsKeys = rhs->getUsedKeys();
        usedKeys.insert(rhsKeys.begin(), rhsKeys.end());

        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
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

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<MathValue>(other)) {
            // Compare the values
            if (lhs && casted->lhs && !lhs->isEqual(casted->lhs)) {
                return false;
            }
            if (rhs && casted->rhs && !rhs->isEqual(casted->rhs)) {
                return false;
            }
            if (operation != casted->operation) {
                return false;
            }

            return true; // Values are equal
        }

        return false; // Not the same type or nullptr
    }


private:
    const std::shared_ptr<Value> lhs;
    const std::shared_ptr<Value> rhs;
    const MathOperation operation;
};

class LengthValue: public Value {
    const std::shared_ptr<Value> value;

public:
    LengthValue(const std::shared_ptr<Value> value): value(value) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<LengthValue>(value->clone());
    }

    std::unordered_set<std::string> getUsedKeys() const override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return std::visit(overloaded {
            [](const std::string &val){
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
            [](const Color &val){
                return (int64_t) 0;
            },
            [](const std::vector<float> &val){
                return (int64_t) val.size();
            },
            [](const std::vector<std::string> &val){
                return (int64_t) val.size();
            },
            [](const std::vector<FormattedStringEntry> &val){
                return (int64_t) val.size();
            },
            [](const std::monostate &val) {
                return (int64_t)0;
            }
        }, value->evaluate(context));
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<LengthValue>(other)) {
            // Compare the value member
            if (value && casted->value && !value->isEqual(casted->value)) {
                return false;
            }

            return true; // Values are equal
        }

        return false; // Not the same type or nullptr
    }

};
