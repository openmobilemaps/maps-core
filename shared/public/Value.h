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
#include "ValueVariant.h"
#include "Tiled2dMapVectorStateManager.h"
#include "ValueKeys.h"
#include "VectorSet.h"
#include <iomanip>
#include <memory>
#include <utility>
#include "Logger.h"

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

struct property_value_mapping : vtzero::property_value_mapping {
    using float_type = double;
    using uint_type = int64_t;
};

class FeatureContext {
public:
    using keyType = std::string;
    using valueType = ValueVariant;
    using mapType = std::vector<std::pair<keyType, valueType>>;

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

    FeatureContext(vtzero::GeomType geomType,
                   mapType propertiesMap,
                   const std::string &stringIdentifier):
    propertiesMap(std::move(propertiesMap)),
    geomType(geomType) {
        size_t hash = 0;
        std::hash_combine(hash, std::hash<std::string>{}(stringIdentifier));
        identifier = hash;

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
        propertiesMap.push_back(std::make_pair(ValueKeys::IDENTIFIER_KEY, int64_t(identifier)));

        switch (geomType) {
            case vtzero::GeomType::LINESTRING: {
                propertiesMap.push_back(std::make_pair(ValueKeys::TYPE_KEY, "LineString"));
                break;
            }
            case vtzero::GeomType::POINT: {
                propertiesMap.push_back(std::make_pair(ValueKeys::TYPE_KEY, "Point"));
                break;
            }
            case vtzero::GeomType::POLYGON: {
                propertiesMap.push_back(std::make_pair(ValueKeys::TYPE_KEY, "Polygon"));
                break;
            }
            case vtzero::GeomType::UNKNOWN: {
                propertiesMap.push_back(std::make_pair(ValueKeys::TYPE_KEY, "Unknown"));
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


// The Evaluation Context is needed to evaluate Value properties.
// Please note that all members are only references; therefore, they have to be kept alive
// as long as the evaluation context is used.
class EvaluationContext {
public:
    const double zoomLevel;
    const double dpFactor;
    const std::shared_ptr<FeatureContext> &feature;
    const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager;

    EvaluationContext(const double zoomLevel,
                      const double dpFactor,
                      const std::shared_ptr<FeatureContext> &feature,
                      const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager) :
        zoomLevel(zoomLevel), dpFactor(dpFactor), feature(feature), featureStateManager(featureStateManager) {}
};


class UsedKeysCollection {
public:
    VectorSet<std::string> usedKeys;
    VectorSet<std::string> featureStateKeys;
    VectorSet<std::string> globalStateKeys;

    UsedKeysCollection() {};

    UsedKeysCollection(const VectorSet<std::string> &usedKeys) : usedKeys(usedKeys) {};

    UsedKeysCollection(const VectorSet<std::string> &usedKeys,
                       const VectorSet<std::string> &featureStateKeys,
                       const VectorSet<std::string> &globalStateKeys)
            : usedKeys(usedKeys),
              featureStateKeys(featureStateKeys),
              globalStateKeys(globalStateKeys) {};

    void includeOther(const UsedKeysCollection &other) {
        usedKeys.insertSet(other.usedKeys);
        featureStateKeys.insertSet(other.featureStateKeys);
        globalStateKeys.insertSet(other.globalStateKeys);
    };

    bool isStateDependant() const {
        return !(featureStateKeys.empty() && globalStateKeys.empty());
    };

    bool containsUsedKey(const std::string &key) const {
        return usedKeys.contains(key);
    }

    bool empty() const {
        return usedKeys.empty() && featureStateKeys.empty() && globalStateKeys.empty();
    }

    bool onlyGlobalStateDependant() {
        return featureStateKeys.empty() && !globalStateKeys.empty();
    }

    bool isGlobalStateDependant() {
        return !globalStateKeys.empty();
    }

    size_t size() const {
        return usedKeys.size() + featureStateKeys.size() + globalStateKeys.size();
    }

    bool covers(const UsedKeysCollection &other) {
        return usedKeys.covers(other.usedKeys) &&
               featureStateKeys.covers(other.featureStateKeys) &&
               globalStateKeys.covers(other.globalStateKeys);
    }

    size_t getHash(const EvaluationContext &context) const {
        size_t hash = 0;
        if (context.feature) {
            for (const auto &[propertyKey, propertyValue] : context.feature->propertiesMap) {
                if (containsUsedKey(propertyKey)) {
                    std::hash_combine(hash, std::hash<FeatureContext::valueType>{}(propertyValue));
                }
            }
        }

        if (context.featureStateManager) {
            if (context.feature) {
                const auto &state = context.featureStateManager->getFeatureState(context.feature->identifier);

                for (const auto &featureStateKey: featureStateKeys) {
                    const auto &value = state.find(featureStateKey);
                    if (value != state.end()) {
                        std::hash_combine(hash, std::hash<ValueVariant>{}(value->second));
                    }
                }
            }
            for (const auto &globalStateKey: globalStateKeys) {
                const auto &value = context.featureStateManager->getGlobalState(globalStateKey);
                std::hash_combine(hash, std::hash<ValueVariant>{}(value));
            }
        }
        
        return hash;
    };
};


class Value {
public:
    Value() {};
    virtual ~Value() = default;

    virtual std::unique_ptr<Value> clone() = 0;

    virtual UsedKeysCollection getUsedKeys() const { return UsedKeysCollection(); };

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

    std::optional<TextTransform> textTransformFromString(const std::string &value) const {
        if (value == "none") {
            return TextTransform::NONE;
        } else if (value == "uppercase") {
            return TextTransform::UPPERCASE;
        }
        return std::nullopt;
    }
    
    virtual bool isGettingPropertyValues() {
        return false;
    }
};

// XXX(matzf): moved out of class scope. Template specialization in class scope _should_ be allowed in C++ since Defect Report CWG 727. But GCC still rejects it (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282).
template<>
inline Vec2F Value::evaluateOr(const EvaluationContext &context, const Vec2F &alternative) const {
    auto const &value = evaluateOr(context, std::vector<float>{ alternative.x, alternative.y });
    return Vec2F(value[0], value[1]);
}

template<>
inline BlendMode Value::evaluateOr(const EvaluationContext &context, const BlendMode &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto blendMode = blendModeFromString(value);
    if (blendMode) {
        return *blendMode;
    }
    return alternative;
}

template<>
inline SymbolZOrder Value::evaluateOr(const EvaluationContext &context, const SymbolZOrder &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto symbolZOrder = symbolZOrderFromString(value);
    if (symbolZOrder) {
        return *symbolZOrder;
    }
    return alternative;
}

template<>
inline Anchor Value::evaluateOr(const EvaluationContext &context, const Anchor &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto anchor = anchorFromString(value);
    if (anchor) {
        return *anchor;
    }
    return alternative;
}

template<>
inline SymbolAlignment Value::evaluateOr(const EvaluationContext &context, const SymbolAlignment &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto alignment = alignmentFromString(value);
    if (alignment) {
        return *alignment;
    }
    return alternative;
}

template<>
inline IconTextFit Value::evaluateOr(const EvaluationContext &context, const IconTextFit &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto textFit = iconTextFitFromString(value);
    if (textFit) {
        return *textFit;
    }
    return alternative;
}

template<>
inline TextJustify Value::evaluateOr(const EvaluationContext &context, const TextJustify &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto anchor = justifyFromString(value);
    if (anchor) {
        return *anchor;
    }
    return alternative;
}

template<>
inline TextSymbolPlacement Value::evaluateOr(const EvaluationContext &context, const TextSymbolPlacement &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto placement = textSymbolPlacementFromString(value);
    if (placement) {
        return *placement;
    }
    return alternative;
}


template<>
inline LineCapType Value::evaluateOr(const EvaluationContext &context, const LineCapType &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto type = capTypeFromString(value);
    if (type) {
        return *type;
    }
    return alternative;
}

template<>
inline TextTransform Value::evaluateOr(const EvaluationContext &context, const TextTransform &alternative) const {
    auto const &value = evaluateOr(context, std::string(""));
    auto type = textTransformFromString(value);
    if (type) {
        return *type;
    }
    return alternative;
}

template<>
inline std::vector<Anchor> Value::evaluateOr(const EvaluationContext &context, const std::vector<Anchor> &alternative) const {
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


template<class ResultType>
class ValueEvaluator {
public:
    ValueEvaluator(const std::shared_ptr<Value> &value) : value(value) {
        if(!value) {
            return;
        }

        updateValue(value);
    }

    ValueEvaluator(const ValueEvaluator& evaluator) : ValueEvaluator(evaluator.getValue()) {};

    std::shared_ptr<Value> getValue() const {
        return value;
    }

    void updateValue(std::shared_ptr<Value> newValue) {
        value = newValue;

        usedKeysCollection = value->getUsedKeys();

        isStatic = usedKeysCollection.empty();
        isZoomDependent = usedKeysCollection.usedKeys.contains("zoom");
        isStateDependant = usedKeysCollection.isStateDependant();
        onlyGlobalStateDependant = usedKeysCollection.onlyGlobalStateDependant();
        isGlobalStateDependant = usedKeysCollection.isGlobalStateDependant();

        lastResults.clear();
    }


    inline ResultType getResult(const EvaluationContext &context, const ResultType &defaultValue) {
        if (!value) {
            return defaultValue;
        }

        if (isStatic) {
            if(!staticValue) {
                staticValue = value->evaluateOr(context, defaultValue);
            }

            return *staticValue;
        }

        if(value->isGettingPropertyValues()) {
            return value->evaluateOr(context, defaultValue);
        }

        if (isZoomDependent || (isStateDependant && !context.featureStateManager->empty())) {
            return value->evaluateOr(context, defaultValue);
        }

        if(onlyGlobalStateDependant) {
            auto currentGlobalId = context.featureStateManager->getCurrentState();

            if(currentGlobalId != globalId) {
                globalValue = value->evaluateOr(context, defaultValue);
                globalId = currentGlobalId;
            }

            if(globalValue) {
                return *globalValue;
            }
        }


        if (isGlobalStateDependant) {
            auto currentGlobalId = context.featureStateManager->getCurrentState();
            if(currentGlobalId != globalId) {
                lastResults.clear();
                globalId = currentGlobalId;
            }
        }

        int64_t identifier = usedKeysCollection.getHash(context);

        std::lock_guard<std::mutex> lock(mutex);

        const auto &lastResultIt = lastResults.find(identifier);
        if (lastResultIt != lastResults.end()) {
            return lastResultIt->second;
        }

        const auto &result = value->evaluateOr(context, defaultValue);
        lastResults.insert({identifier, result});

        return result;
    }

private:
    std::mutex mutex;
    std::shared_ptr<Value> value;
    UsedKeysCollection usedKeysCollection;
    std::optional<ResultType> staticValue;
    std::optional<ResultType> globalValue;


    bool isStatic = false;
    bool isZoomDependent = false;
    bool isStateDependant = false;
    bool onlyGlobalStateDependant = false;
    bool isGlobalStateDependant = false;

    int64_t globalId = -1;

    std::unordered_map<uint64_t, ResultType> lastResults;
};

class GetPropertyValue : public Value {
public:
    GetPropertyValue(const std::string key) : key(key) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<GetPropertyValue>(key);
    }

    UsedKeysCollection getUsedKeys() const override {
        return UsedKeysCollection({ key });
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        if (key == "zoom") {
            return context.zoomLevel ? context.zoomLevel : 0.0;
        }

        return context.feature->getValue(key);
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<GetPropertyValue>(other)) {
            return casted->key == key;
        }
        return false;
    };

    bool isGettingPropertyValues() override {
        return true;
    }

private:
    const std::string key;
};

class FeatureStateValue : public Value {
public:
    FeatureStateValue(const std::string key) : key(key) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<FeatureStateValue>(key);
    }

    UsedKeysCollection getUsedKeys() const override {
        return UsedKeysCollection({}, {key}, {});
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        const auto& stateMap = context.featureStateManager->getFeatureState(context.feature->identifier);
        const auto& result = stateMap.find(key);
        if (result != stateMap.end()) {
            if(!std::holds_alternative<std::monostate>(result->second)) {
                return result->second;
            }
        }
        return std::monostate();
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<FeatureStateValue>(other)) {
            return casted->key == key;
        }
        return false;
    };
private:
    const std::string key;
};

class GlobalStateValue : public Value {
public:
    GlobalStateValue(const std::string key) : key(key) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<GlobalStateValue>(key);
    }

    UsedKeysCollection getUsedKeys() const override {
        return UsedKeysCollection({}, {}, {key});
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return context.featureStateManager->getGlobalState(key);
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<GlobalStateValue>(other)) {
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

    UsedKeysCollection getUsedKeys() const override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return std::visit(valueVariantToStringVisitor, value->evaluate(context));
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

    UsedKeysCollection getUsedKeys() const override {
        if (std::holds_alternative<std::string>(value)) {
            std::string res = std::get<std::string>(value);
            VectorSet<std::string> usedKeys = { res };

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

            return UsedKeysCollection(usedKeys);

        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            const auto& res = std::get<std::vector<std::string>>(value);
            if (!res.empty() && *res.begin() == "zoom") {
                return UsedKeysCollection({ "zoom" });
            }
            return UsedKeysCollection();
        } else {
            return UsedKeysCollection();
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
            const auto& res = std::get<std::vector<std::string>>(value);
            if (!res.empty() && *res.begin() == "zoom") {
                return context.zoomLevel ? context.zoomLevel : 0.0;
            }

            return value;
        } else {

            return value;
        }

    };

    bool isGettingPropertyValues() override {
        return true;
    }

    bool isStaticNumber() {
        return std::holds_alternative<double>(value) ||
               std::holds_alternative<int64_t>(value);
    }

    double getStaticDouble() {
        if(std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }

        if(std::holds_alternative<int64_t>(value)) {
            return std::get<int64_t>(value);
        }

        // should be checked by isStaticNumber
        assert(false);
        return 0;
    }

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

    UsedKeysCollection getUsedKeys() const override {
        return UsedKeysCollection({ key });
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

class HasNotPropertyValue : public Value {
public:
    HasNotPropertyValue(const std::string key) : key(key) {};

    std::unique_ptr<Value> clone() override {
        return std::make_unique<HasNotPropertyValue>(key);
    }

    UsedKeysCollection getUsedKeys() const override {
        return UsedKeysCollection({ key });
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return !context.feature->contains(key);
    };

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<HasNotPropertyValue>(other)) {
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

    UsedKeysCollection getUsedKeys() const override { return value->getUsedKeys(); }

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
    static double interpolationFactor(const double &base, const double &rangeFactor, const double &x, const double &a) {
        double progress = std::max(0.0, x - a);
        if(base == 1.0) {
            return rangeFactor * progress;
        } else {
            return rangeFactor * (std::pow(base, progress) - 1);
        }
    }

    static double rangeFactor(const double &base, const double &b, const double &a) {
        double range = b - a;

        if(base == 1.0) {
            return 1.0 / range;
        } else {
            return 1.0 / (std::pow(base, range) - 1.0);
        }
    }
};

class InterpolatedValue : public Value {
public:
    InterpolatedValue(double interpolationBase, const std::vector<std::pair<double, std::shared_ptr<Value>>> &steps)
    : interpolationBase(interpolationBase), steps(steps) {
        precomputeRanges();
        checkSpecialCase();
    }

    std::unique_ptr<Value> clone() override {
        std::vector<std::pair<double, std::shared_ptr<Value>>> clonedSteps;
        for (const auto &[step, value] : steps) {
            clonedSteps.emplace_back(step, value->clone());
        }
        return std::make_unique<InterpolatedValue>(interpolationBase, std::move(clonedSteps));
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys = UsedKeysCollection({ "zoom" });
        for (auto const &step: steps) {
            auto const stepKeys = step.second->getUsedKeys();
            usedKeys.includeOther(stepKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        if(isFast) {
            int maxStepInd = (int)fastSteps.size() - 1;
            for (int i = 0; i < maxStepInd; i++) {
                const auto &nextStep = fastSteps[i + 1];
                double nS = nextStep.first;

                if (nS >= context.zoomLevel) {
                    const auto &prevStep = fastSteps[i];
                    auto f = ExponentialInterpolation::interpolationFactor(interpolationBase, rangeFactors[i], context.zoomLevel, prevStep.first);
                    return prevStep.second + (nextStep.second - prevStep.second) * f;
                }
            }

            return fastSteps[maxStepInd].second;
        }

        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = nextStep.first;

            if (nS >= context.zoomLevel) {
                const auto &prevStep = steps[i];
                double pS = prevStep.first;
                const ValueVariant &pV = prevStep.second->evaluate(context);
                const ValueVariant &nV = nextStep.second->evaluate(context);
                return interpolate(ExponentialInterpolation::interpolationFactor(interpolationBase, rangeFactors[i], context.zoomLevel, pS), pV, nV);
            }
        }

        return steps[maxStepInd].second->evaluate(context);
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
                if (casted->steps[i].first != steps[i].first) {
                    return false;
                }
                if ((casted->steps[i].second && steps[i].second) &&
                    !casted->steps[i].second->isEqual(steps[i].second)) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    ValueVariant interpolate(const double &interpolationFactor, const ValueVariant &yBase, const ValueVariant &yTop) const {

        auto yBaseIndex = yBase.index();
        auto yTopIndex = yTop.index();

        if (yBaseIndex == 2 && yTopIndex == yBaseIndex) {
            int64_t base = std::get<2>(yBase);
            int64_t top = std::get<2>(yTop);

            return base + (top - base) * interpolationFactor;
        }

        if (yBaseIndex == 1 && yTopIndex == yBaseIndex) {
            double base = std::get<1>(yBase);
            double top = std::get<1>(yTop);

            return base + (top - base) * interpolationFactor;
        }

        if (yBaseIndex == 2 && yTopIndex == 1) {
            double base = (double)std::get<2>(yBase);
            double top = std::get<1>(yTop);

            return base + (top - base) * interpolationFactor;
        }

        if (yBaseIndex == 1 && yTopIndex == 2) {
            double base = std::get<1>(yBase);
            double top = (double)std::get<2>(yTop);

            return base + (top - base) * interpolationFactor;
        }

        if (yBaseIndex == 5 && yTopIndex == yBaseIndex) {
            const std::vector<float> &base = std::get<5>(yBase);
            const std::vector<float> &top = std::get<5>(yTop);

            assert(base.size() == top.size());
            std::vector<float> result(base.size(), 0.0);
            for (size_t i = 0; i != base.size(); i++) {
                result[i] = base[i] + (top[i] - base[i]) * interpolationFactor;
            }
            return result;
        }

        if (yBaseIndex == 4 && yBaseIndex == yTopIndex) {
            const Color &yBaseC = std::get<4>(yBase);
            const Color &yTopC = std::get<4>(yTop);
            return Color(yBaseC.r + (yTopC.r - yBaseC.r) * interpolationFactor,
                         yBaseC.g + (yTopC.g - yBaseC.g) * interpolationFactor,
                         yBaseC.b + (yTopC.b - yBaseC.b) * interpolationFactor,
                         yBaseC.a + (yTopC.a - yBaseC.a) * interpolationFactor);
        }

        if (interpolationFactor < 0.5) {
            return yBase;
        } else {
            return yTop;
        }
    }

    void checkSpecialCase() {
        bool allStaticDoubles = true;
        for(const auto &step : steps) {
            const auto& staticVal = std::dynamic_pointer_cast<StaticValue>(step.second);
            if(staticVal) {
                if(!staticVal->isStaticNumber()) {
                    allStaticDoubles = false;
                    break;
                }
            } else {
                allStaticDoubles = false;
                break;
            }
        }

        isFast = allStaticDoubles;

        if(isFast) {
            for(const auto &step : steps) {
                const auto& staticVal = std::dynamic_pointer_cast<StaticValue>(step.second);
                if(staticVal) {
                    fastSteps.emplace_back(step.first, staticVal->getStaticDouble());
                }
            }

            steps.clear();
        }
    }

    void precomputeRanges() {
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            rangeFactors.push_back(ExponentialInterpolation::rangeFactor(interpolationBase, steps[i + 1].first, steps[i].first));
        }
    }

private:
    double interpolationBase;
    std::vector<std::pair<double, std::shared_ptr<Value>>> steps;
    std::vector<std::pair<double, double>> fastSteps;
    std::vector<double> rangeFactors;
    bool isFast;
};

class BezierInterpolatedValue : public Value {
public:
    BezierInterpolatedValue(double x1, double y1, double x2, double y2, const std::vector<std::pair<double, std::shared_ptr<Value>>> &steps)
            : bezier(x1, y1, x2, y2), steps(steps) {}

    BezierInterpolatedValue(const UnitBezier &bezier, const std::vector<std::pair<double, std::shared_ptr<Value>>> &steps)
            : bezier(bezier), steps(steps) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::pair<double, std::shared_ptr<Value>>> clonedSteps;
        for (const auto &[step, value] : steps) {
            clonedSteps.emplace_back(step, value->clone());
        }
        return std::make_unique<BezierInterpolatedValue>(bezier, std::move(clonedSteps));
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys = UsedKeysCollection({"zoom"});
        for (auto const &step: steps) {
            auto const stepKeys = step.second->getUsedKeys();
            usedKeys.includeOther(stepKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = nextStep.first;
            if (nS >= context.zoomLevel) {
                const auto &prevStep = steps[i];
                double pS = prevStep.first;
                ValueVariant pV = prevStep.second->evaluate(context);
                ValueVariant nV = nextStep.second->evaluate(context);

                auto factor = bezier.solve(1.0 - (nS - context.zoomLevel) / (nS - pS), 1e-6);
                return interpolate(factor, pV, nV);
            }
        }

        int index = (steps.size() > 0 && context.zoomLevel <= steps[0].first) ? 0 : maxStepInd;
        return steps[index].second->evaluate(context);
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
                if (casted->steps[i].first != steps[i].first) {
                    return false;
                }
                if ((casted->steps[i].second && steps[i].second) && !casted->steps[i].second->isEqual(steps[i].second)) {
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
    const std::vector<std::pair<double, std::shared_ptr<Value>>> steps;
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
              const std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops,
              std::shared_ptr<Value> defaultValue) : compareValue(compareValue), stops(stops), defaultValue(defaultValue) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> clonedStops;
        for (const auto &[v1, v2] : stops) {
            clonedStops.emplace_back(v1->clone(), v2->clone());
        }
        return std::make_unique<StepValue>(compareValue->clone(), std::move(clonedStops), defaultValue->clone());
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        
        auto const compareValueKeys = compareValue->getUsedKeys();
        usedKeys.includeOther(compareValueKeys);

        auto const defaultValueKeys = defaultValue->getUsedKeys();
        usedKeys.includeOther(defaultValueKeys);

        for (auto const &stop: stops) {
            auto const setKeys = stop.second->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        const auto &compareValue_ = compareValue->evaluate(context);

        for (auto it = stops.begin(); it != stops.end(); it++) {
            if (ValueVariantCompareHelper::compare((*it).first->evaluate(context), compareValue_, PropertyCompareType::GREATER)) {
                if (it != stops.begin()) {
                    return (*std::prev(it)).second->evaluate(context);
                } else {
                    return defaultValue->evaluate(context);
                }
            }
        }
        return (*stops.rbegin()).second->evaluate(context);
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

                // Compare the first value in the pair
                if (thisStop.first && otherStop.first && !thisStop.first->isEqual(otherStop.first)) {
                    return false;
                }

                // Compare the second value in the pair
                if (thisStop.second && otherStop.second && !thisStop.second->isEqual(otherStop.second)) {
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
    std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops;
    std::shared_ptr<Value> defaultValue;
};

class CaseValue : public Value {
public:
    CaseValue(const std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases, std::shared_ptr<Value> defaultValue) : cases(cases), defaultValue(defaultValue) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> clonedCases;
        for (const auto &[v1, v2] : cases) {
            clonedCases.emplace_back(v1->clone(), v2->clone());
        }
        return std::make_unique<CaseValue>(std::move(clonedCases), defaultValue->clone());
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto const defaultValueKeys = defaultValue->getUsedKeys();
        usedKeys.includeOther(defaultValueKeys);

        for (auto const &[condition, value]: cases) {
            if (condition) {
                auto const conditionKeys = condition->getUsedKeys();
                usedKeys.includeOther(conditionKeys);
            }

            auto const valueKeys = value->getUsedKeys();
            usedKeys.includeOther(valueKeys);
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

                // Compare the first value in the pair (condition)
                if (thisCase.first && otherCase.first && !thisCase.first->isEqual(otherCase.first)) {
                    return false;
                }

                // Compare the second value in the pair (value)
                if (!thisCase.second->isEqual(otherCase.second)) {
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
    const std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases;
    const std::shared_ptr<Value> defaultValue;
};


class ToNumberValue: public Value {
    const std::shared_ptr<Value> value;

public:
    ToNumberValue(const std::shared_ptr<Value> value): value(value) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<ToNumberValue>(value->clone());
    }

    UsedKeysCollection getUsedKeys() const override {
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

class ToBooleanValue: public Value {
    const std::shared_ptr<Value> value;

public:
    ToBooleanValue(const std::shared_ptr<Value> value): value(value) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<ToBooleanValue>(value->clone());
    }

    UsedKeysCollection getUsedKeys() const override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        return std::visit(overloaded {
                [](const std::string &val){
                    return !val.empty();
                },
                [](double val){
                    return val != 0.0 && !std::isnan(val);
                },
                [](int64_t val){
                    return val != 0 && !std::isnan(val);
                },
                [](bool val){
                    return val;
                },
                [](const Color &val){
                    return true;
                },
                [](const std::vector<float> &val){
                    return true;
                },
                [](const std::vector<std::string> &val){
                    return true;
                },
                [](const std::vector<FormattedStringEntry> &val){
                    return true;
                },
                [](const std::monostate &val) {
                    return false;
                }
        }, value->evaluate(context));
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<ToBooleanValue>(other)) {
            // Compare the value member
            if (value && casted->value && !value->isEqual(casted->value)) {
                return false;
            }
            return true; // All members are equal
        }
        return false; // Not the same type or nullptr
    }
};

class BooleanValue: public Value {
    const std::vector<std::shared_ptr<Value>> values;

public:
    BooleanValue(const std::shared_ptr<Value> value): values({value }) {}

    BooleanValue(const std::vector<std::shared_ptr<Value>> values): values(values) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::shared_ptr<Value>> clonedValues;
        for (const auto &value: values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<BooleanValue>(clonedValues);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        for (const auto &value: values) {
            const auto valueKeys = value->getUsedKeys();
            usedKeys.includeOther(valueKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        for (const auto &value: values) {
            auto variant = value->evaluate(context);
            if (std::holds_alternative<bool>(variant)) {
                return std::get<bool>(variant);
            }
        }
        return false;
    };

    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<BooleanValue>(other)) {
            // Compare the value members
            for (const auto &value: values) {
                bool found = false;
                for (auto const &castedValue: casted->values) {
                    if (value && castedValue && value->isEqual(castedValue)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
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

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto const compareValueKeys = compareValue->getUsedKeys();
        usedKeys.includeOther(compareValueKeys);

        auto const defaultValueKeys = defaultValue->getUsedKeys();
        usedKeys.includeOther(defaultValueKeys);

        for (auto const &i: valueMapping) {
            auto const valueKeys = i.second->getUsedKeys();
            usedKeys.includeOther(valueKeys);
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

    UsedKeysCollection getUsedKeys() const override {
        return UsedKeysCollection({ key });
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
            default:
#if __cplusplus >= 202302L
                std::unreachable();
#else
                __builtin_unreachable();
#endif
        }
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.includeOther(lhsKeys);

        if (rhs) {
            auto const rhsKeys = rhs->getUsedKeys();
            usedKeys.includeOther(rhsKeys);
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
            default:
#if __cplusplus >= 202302L
                std::unreachable();
#else
                __builtin_unreachable();
#endif
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
    AllValue(const std::vector<std::shared_ptr<Value>> values) : values(values) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::shared_ptr<Value>> clonedValues;
        for (const auto &value : values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<AllValue>(std::move(clonedValues));
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        for (auto const &value: values) {
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        for (auto const &value: values) {
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
    const std::vector<std::shared_ptr<Value>> values;
};

class AnyValue: public Value {
public:
    AnyValue(const std::vector<std::shared_ptr<Value>> values) : values(values) {}

    std::unique_ptr<Value> clone() override {
        std::vector<std::shared_ptr<Value>> clonedValues;
        for (const auto &value : values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<AnyValue>(std::move(clonedValues));
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        for (auto const &value: values) {
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
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
    const std::vector<std::shared_ptr<Value>> values;
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

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.includeOther(lhsKeys);

        auto const rhsKeys = rhs->getUsedKeys();
        usedKeys.includeOther(rhsKeys);

        return usedKeys;
    }

 ValueVariant evaluate(const EvaluationContext &context) const override {
     auto const &lhsValue = lhs->evaluate(context);
     auto const &rhsValue = rhs->evaluate(context);

     // Do not try to compare not existent values. Parent value will handle default case.
     if (std::holds_alternative<std::monostate>(lhsValue) || std::holds_alternative<std::monostate>(rhsValue)) {
         switch (type) {
             case PropertyCompareType::EQUAL:
                 return lhs == rhs;
             case PropertyCompareType::NOTEQUAL:
                 return lhs != rhs;
             case PropertyCompareType::LESS:
                 return std::monostate();
             case PropertyCompareType::LESSEQUAL:
                 return std::monostate();
             case PropertyCompareType::GREATER:
                 return std::monostate();
             case PropertyCompareType::GREATEREQUAL:
                 return std::monostate();
         }
     }

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
    const std::shared_ptr<Value> dynamicValues;
    const std::string key;
public:
    InFilter(const std::string &key, const std::unordered_set<ValueVariant> values, const std::shared_ptr<Value> dynamicValues) :values(values), key(key), dynamicValues(dynamicValues) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<InFilter>(key, values, dynamicValues);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys = UsedKeysCollection({ key });
        if (dynamicValues) {
            auto const setKeys = dynamicValues->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        auto const &value = context.feature->getValue(key);
        if (values.count(value) != 0) {
            return true;
        }
        if (dynamicValues) {
            bool isString = std::holds_alternative<std::string>(value);
            bool isDouble = std::holds_alternative<double>(value);
            bool isInt = std::holds_alternative<int64_t>(value);
            if (!isString && !isDouble && !isInt) {
                return false;
            }

            auto const &dynamicVariants = dynamicValues->evaluate(context);

            if (isString && std::holds_alternative<std::vector<std::string>>(dynamicVariants)) {
                const auto& stringValue = std::get<std::string>(value);
                const auto& strings = std::get<std::vector<std::string>>(dynamicVariants);
                for (auto const &string: strings) {
                    if (string == stringValue) {
                        return true;
                    }
                }
            } else if ((isDouble || isInt) && std::holds_alternative<std::vector<float>>(dynamicVariants)) {
                double doubleValue;
                if (isDouble) {
                    doubleValue = std::get<double>(value);
                } else {
                    doubleValue = std::get<int64_t>(value);
                }

                const auto& floats = std::get<std::vector<float>>(dynamicVariants);
                for (auto const &f: floats) {
                    if (f == doubleValue) {
                        return true;
                    }
                }
            }
        }

        return false;
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

            if (dynamicValues != casted->dynamicValues) {
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
    const std::shared_ptr<Value> dynamicValues;
    const std::string key;
public:
    NotInFilter(const std::string &key, const std::unordered_set<ValueVariant> values, const std::shared_ptr<Value> dynamicValues) :values(values), key(key), dynamicValues(dynamicValues) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<NotInFilter>(key, values, dynamicValues);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys = UsedKeysCollection({ key });
        if (dynamicValues) {
            auto const setKeys = dynamicValues->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        auto const &value = context.feature->getValue(key);
        if (values.count(value) != 0) {
            return false;
        }

        if (dynamicValues) {
            bool isString = std::holds_alternative<std::string>(value);
            bool isDouble = std::holds_alternative<double>(value);
            bool isInt = std::holds_alternative<int64_t>(value);
            if (!isString && !isDouble && !isInt) {
                return true;
            }

            auto const dynamicVariants = dynamicValues->evaluate(context);

            if (isString && std::holds_alternative<std::vector<std::string>>(dynamicVariants)) {
                const auto &stringValue = std::get<std::string>(value);
                const auto &strings = std::get<std::vector<std::string>>(dynamicVariants);
                for (auto const &string: strings) {
                    if (string == stringValue) {
                        return false;
                    }
                }
            } else if ((isDouble || isInt) && std::holds_alternative<std::vector<float>>(dynamicVariants)) {
                double doubleValue;
                if (isDouble) {
                    doubleValue = std::get<double>(value);
                } else {
                    doubleValue = std::get<int64_t>(value);
                }

                const auto& floats = std::get<std::vector<float>>(dynamicVariants);
                for (auto const &f: floats) {
                    if (f == doubleValue) {
                        return false;
                    }
                }
            }
        }

        return true;
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

            if (dynamicValues != casted->dynamicValues) {
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

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        for (auto const &wrapper: values) {
            auto const setKeys = wrapper.value->getUsedKeys();
            usedKeys.includeOther(setKeys);
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

class NumberFormatValue : public Value {
public:
    NumberFormatValue(const std::shared_ptr<Value> &value, int minFractionDigits = 0, int maxFactionDigits = 0)
            : value(value), minFractionDigits(minFractionDigits), maxFractionDigits(maxFactionDigits) {}

    std::unique_ptr<Value> clone() override {
        return std::make_unique<NumberFormatValue>(value->clone(), minFractionDigits, maxFractionDigits);
    }

    UsedKeysCollection getUsedKeys() const override {
        return value->getUsedKeys();
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        auto valueVariant = value->evaluate(context);

        std::optional<double> numberValue = std::nullopt;
        if (std::holds_alternative<double>(valueVariant)) {
            numberValue = std::get<double>(valueVariant);
        } else if (std::holds_alternative<int64_t>(valueVariant)) {
            numberValue = std::get<int64_t>(valueVariant);
        } else if (std::holds_alternative<std::string>(valueVariant)) {
            // try to parse the string
            try {
                numberValue = std::stod(std::get<std::string>(valueVariant));
            } catch (const std::invalid_argument &e) {
                return std::monostate();
            }
        }

        if (!numberValue.has_value()) {
            return std::monostate();
        }

        double factor = std::pow(10, maxFractionDigits);
        double roundedVal = std::round(*numberValue * factor) / factor;

        std::stringstream resultStream;
        resultStream << std::fixed << std::setprecision(maxFractionDigits) << roundedVal;
        std::string result = resultStream.str();
        size_t decimalPointPos = result.find('.');
        if (decimalPointPos != std::string::npos) {
            while (result.back() == '0' || result.length() - decimalPointPos - 1 > maxFractionDigits) {
                result.pop_back();
            }
        }
        if (maxFractionDigits == 0) {
            if (result.back() == '.') {
                result.pop_back();
            }
        } else if (minFractionDigits > 0) {
            if (decimalPointPos == std::string::npos) {
                decimalPointPos = result.length();
                result += '.';
            }

            size_t fractionDigits = result.length() - decimalPointPos - 1;
            size_t zerosToAdd = minFractionDigits - fractionDigits;

            if (zerosToAdd > 0) {
                result.append(zerosToAdd, '0');
            }
        }

        return result;
    }

    bool isEqual(const std::shared_ptr<Value> &other) const override {
        if (auto casted = std::dynamic_pointer_cast<NumberFormatValue>(other)) {
            if (value && casted->value && !value->isEqual(casted->value)) {
                return false;
            }
            if (minFractionDigits != casted->minFractionDigits || maxFractionDigits != casted->maxFractionDigits) {
                return false;
            }
            return true;
        }
        return false;
    }

private:
    const std::shared_ptr<Value> value;
    int minFractionDigits = 0;
    int maxFractionDigits = 0;
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
    }

    std::unique_ptr<Value> clone() override {
        return std::make_unique<MathValue>(lhs->clone(), rhs ? rhs->clone() : nullptr, operation);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto const lhsKeys = lhs->getUsedKeys();
        usedKeys.includeOther(lhsKeys);

        if (rhs) {
            auto const rhsKeys = rhs->getUsedKeys();
            usedKeys.includeOther(rhsKeys);
        }

        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        auto const lhsValue = lhs->evaluateOr(context, (double) 0.0);
        auto const rhsValue = rhs ? rhs->evaluateOr(context, (double) 0.0) : 0;
        switch (operation) {
            case MathOperation::MINUS:
                if (!rhs) { return 0.0 - lhsValue; }
                return lhsValue - rhsValue;
            case MathOperation::PLUS:
                return lhsValue + rhsValue;
            case MathOperation::MULTIPLY:
                return lhsValue * rhsValue;
            case MathOperation::DIVIDE:
                return lhsValue / rhsValue;
            case MathOperation::MODULO:
                return std::fmod(lhsValue, rhsValue);
            case MathOperation::POWER:
                return std::pow(lhsValue,  rhsValue);
            default:
#if __cplusplus >= 202302L
                std::unreachable();
#else
                __builtin_unreachable();
#endif
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

    UsedKeysCollection getUsedKeys() const override {
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

class CoalesceValue : public Value {
private:
    const std::vector<std::shared_ptr<Value>> values;
public:
    CoalesceValue(const std::vector<std::shared_ptr<Value>> &values): values(values){}

    std::unique_ptr<Value> clone() override {
        std::vector<std::shared_ptr<Value>> clonedValues;
        for (const auto &value: values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<CoalesceValue>(clonedValues);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        for (const auto &value: values) {
            const auto valueKeys = value->getUsedKeys();
            usedKeys.includeOther(valueKeys);
        }
        return usedKeys;
    }


    ValueVariant evaluate(const EvaluationContext &context) const override {
        for (const auto &value: values) {
            auto result = value->evaluate(context);
            if (!std::holds_alternative<std::monostate>(result)) {
                return result;
            }
        }
        return std::monostate();
    };


    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<CoalesceValue>(other)) {
            // Compare the value members
            for (const auto &value: values) {
                bool found = false;
                for (auto const &castedValue: casted->values) {
                    if (value && castedValue && value->isEqual(castedValue)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true; // All members are equal
        }
        return false; // Not the same type or nullptr
    }

    virtual bool isGettingPropertyValues() override {
        for (const auto &value: values) {
            if(!value->isGettingPropertyValues()) {
                return false;
            }
        }

        return true;
    }
};

class ArrayValue : public Value {
private:
    const std::vector<std::shared_ptr<Value>> values;
public:
    ArrayValue(const std::vector<std::shared_ptr<Value>> &values): values(values){}

    std::unique_ptr<Value> clone() override {
        std::vector<std::shared_ptr<Value>> clonedValues;
        for (const auto &value: values) {
            clonedValues.push_back(value->clone());
        }
        return std::make_unique<ArrayValue>(clonedValues);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;
        for (const auto &value: values) {
            const auto valueKeys = value->getUsedKeys();
            usedKeys.includeOther(valueKeys);
        }
        return usedKeys;
    }

    ValueVariant evaluate(const EvaluationContext &context) const override {
        std::vector<std::string> stringValues;
        std::vector<float> floatValues;

        for (const auto &value: values) {
            auto v = value->evaluate(context);

            if(std::holds_alternative<int64_t>(v)) {
                floatValues.push_back(std::get<int64_t>(v));
            } else if(std::holds_alternative<double>(v)) {
                floatValues.push_back(std::get<double>(v));
            } else if(std::holds_alternative<std::string>(v)) {
                stringValues.push_back(std::get<std::string>(v));
            }
        }

        if(floatValues.size() == values.size()) {
            return floatValues;
        }

        if(stringValues.size() == values.size()) {
            return stringValues;
        }

        return std::monostate();
    };


    bool isEqual(const std::shared_ptr<Value>& other) const override {
        if (auto casted = std::dynamic_pointer_cast<ArrayValue>(other)) {
            // Compare the value members
            for (const auto &value: values) {
                bool found = false;
                for (auto const &castedValue: casted->values) {
                    if (value && castedValue && value->isEqual(castedValue)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true; // All members are equal
        }
        return false; // Not the same type or nullptr
    }

    virtual bool isGettingPropertyValues() override {
        for (const auto &value: values) {
            if(!value->isGettingPropertyValues()) {
                return false;
            }
        }

        return true;
    }
};

