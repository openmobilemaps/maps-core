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
#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <vtzero/vector_tile.hpp>
#include <cmath>
#include <variant>

class FeatureContext {
    using keyType = std::string;
    using valueType = std::variant<std::string, float, double, int64_t, uint64_t, bool>;
    using mapType = std::unordered_map<keyType, valueType>;
public:
    mapType propertiesMap;

    std::string fClass;
    std::string fSubClass;
    std::string brunnel;
    uint64_t width;

    // not included in differentiation between feature contexts
    uint64_t layer;

    vtzero::GeomType geomType;

    FeatureContext() {}

    FeatureContext(vtzero::feature const &feature) {
        geomType = feature.geometry_type();

        propertiesMap = vtzero::create_properties_map<mapType>(feature);

        fClass = getString("class");
        fSubClass = getString("subclass");
        brunnel = getString("brunnel");
        width = getUInt("width");
        layer = getUInt("layer");
    };

    std::string getString(const std::string &key) const {
        auto it = propertiesMap.find(key);
        if(it == propertiesMap.end()) { return ""; }

        auto &variant = it->second;
        if (!std::holds_alternative<std::string>(variant)) {
            return "";
        }
        return std::get<std::string>(variant);
    }

    uint64_t getUInt(const std::string &key) const {
        auto it = propertiesMap.find(key);
        if(it == propertiesMap.end()) { return 0; }

        auto &variant = it->second;

        if (!std::holds_alternative<uint64_t>(variant)) {
            return 0;
        }
        return std::get<uint64_t>(variant);
    }

    bool operator==(const FeatureContext &o) const { return geomType == o.geomType &&
        fClass == o.fClass &&
        fSubClass == o.fSubClass &&
        brunnel == o.brunnel &&
        width == o.width; }
};


class EvaluationContext {
public:
    std::optional<double> zoomLevel;
    const FeatureContext &feature;

    EvaluationContext(std::optional<double> zoomLevel,
                      const FeatureContext &feature) : zoomLevel(zoomLevel), feature(feature) {};
};

template<typename T>
class Value {
public:
    Value() {};

    virtual T evaluate(const EvaluationContext &context) = 0;
};

template<typename T>
class StaticValue : public Value<T> {
public:
    StaticValue(const T value) : value(value) {};

    T evaluate(const EvaluationContext &context) override {
        return value;
    };
private:
    const T value;
};

class ExponentialInterpolation {
public:
    static double interpolationFactor(const double &base, const double &x, const double &a, const double &b) {
        double range = b - a;
        double progress = std::max(0.0, x - a);
        return (base == 1.0) ? (progress / range) : (std::pow(base, progress) - 1) / (std::pow(base, range) - 1);
    }
};

template<typename T>
class InterpolatedValue : public Value<T> {
public:
    InterpolatedValue(double interpolationBase, const std::vector<std::tuple<double, std::shared_ptr<Value<T>>>> steps)
            : interpolationBase(interpolationBase), steps(steps) {}

    T evaluate(const EvaluationContext &context) override {
        double zoom = context.zoomLevel.has_value() ? context.zoomLevel.value() : 0.0;
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = std::get<0>(nextStep);
            if (nS >= zoom) {
                const auto &prevStep = steps[i];
                double pS = std::get<0>(prevStep);
                T pV = std::get<1>(prevStep)->evaluate(context);
                T nV = std::get<1>(nextStep)->evaluate(context);
                return interpolate(ExponentialInterpolation::interpolationFactor(interpolationBase, zoom, pS, nS), pV, nV);
            }
        }
        const auto last = std::get<1>(steps[maxStepInd]);
        return last->evaluate(context);
    }

protected:
    virtual T interpolate(const double &interpolationFactor, const T &yBase, const T &yTop) = 0;

private:
    double interpolationBase;
    std::vector<std::tuple<double, std::shared_ptr<Value<T>>>> steps;
};

template<typename T>
class InterpolatedNumericValue : public InterpolatedValue<T> {
public:
    using InterpolatedValue<T>::InterpolatedValue;
protected:
    T interpolate(const double &interpolationFactor, const T &yBase, const T &yTop) override {
        return yBase + (yTop - yBase) * interpolationFactor;
    }
};

template<typename T>
class BezierInterpolatedValue : public Value<T> {
public:
    BezierInterpolatedValue(double x1, double y1, double x2, double y2, const std::vector<std::tuple<double, std::shared_ptr<Value<T>>>> steps)
            : bezier(x1, y1, x2, y2), steps(steps) {}

    T evaluate(const EvaluationContext &context) override {
        double zoom = context.zoomLevel.has_value() ? context.zoomLevel.value() : 0.0;
        int maxStepInd = (int)steps.size() - 1;
        for (int i = 0; i < maxStepInd; i++) {
            const auto &nextStep = steps[i + 1];
            double nS = std::get<0>(nextStep);
            if (nS >= zoom) {
                const auto &prevStep = steps[i];
                double pS = std::get<0>(prevStep);
                T pV = std::get<1>(prevStep)->evaluate(context);
                T nV = std::get<1>(nextStep)->evaluate(context);

                auto factor = bezier.solve(1.0 - (nS - zoom) / (nS - pS), 1e-6);
                return interpolate(factor, pV, nV);
            }
        }

        int index = (steps.size() > 0 && zoom <= double(std::get<0>(steps[0]))) ? 0 : maxStepInd;
        const auto step = std::get<1>(steps[index]);
        return step->evaluate(context);
    }

protected:
    virtual T interpolate(const double &interpolationFactor, const T &yBase, const T &yTop) = 0;

private:
    UnitBezier bezier;
    std::vector<std::tuple<double, std::shared_ptr<Value<T>>>> steps;
};

template<typename T>
class BezierInterpolatedNumericValue : public BezierInterpolatedValue<T> {
public:
    using BezierInterpolatedValue<T>::BezierInterpolatedValue;
protected:
    T interpolate(const double &interpolationFactor, const T &yBase, const T &yTop) override {
        return yBase + (yTop - yBase) * interpolationFactor;
    }
};

class InterpolatedColorValue : public InterpolatedValue<Color> {
public:
    using InterpolatedValue<Color>::InterpolatedValue;

protected:
    Color interpolate(const double &interpolationFactor, const Color &yBase, const Color &yTop) override {
        return Color(yBase.r + (yTop.r - yBase.r) * interpolationFactor,
                     yBase.g + (yTop.g - yBase.g) * interpolationFactor,
                     yBase.b + (yTop.b - yBase.b) * interpolationFactor,
                     yBase.a + (yTop.a - yBase.a) * interpolationFactor);
    }
};

template<typename T>
class StopValue : public Value<T> {
public:
    StopValue(const std::vector<std::tuple<double, std::shared_ptr<Value<T>>>> stops) : stops(stops) {}

    T evaluate(const EvaluationContext &context) override {
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
    std::vector<std::tuple<double, std::shared_ptr<Value<T>>>> stops;
};

template<typename T>
class LambdaValue : public Value<T> {
public:
    LambdaValue(const std::function<T(EvaluationContext context)> lambda) : lambda(lambda) {};

    T evaluate(const EvaluationContext &context) override {
        return lambda(context);
    };
private:
    const std::function<T(const EvaluationContext &context)> lambda;
};


class ClassInFilter : public Value<bool> {
private:
    const std::unordered_set<std::string> classes;
public:
    ClassInFilter(const std::unordered_set<std::string> classes) : classes(classes) {}

    bool evaluate(const EvaluationContext &context) override {
        return classes.count(context.feature.fClass) != 0;
    };
};

class ClassNotInFilter : public Value<bool> {
private:
    const std::unordered_set<std::string> classes;
public:
    ClassNotInFilter(const std::unordered_set<std::string> classes) : classes(classes) {}

    bool evaluate(const EvaluationContext &context) override {
        return classes.count(context.feature.fClass) == 0;
    };
};

class SubClassInFilter : public Value<bool> {
private:
    const std::unordered_set<std::string> subClasses;
public:
    SubClassInFilter(const std::unordered_set<std::string> subClasses) : subClasses(subClasses) {}

    bool evaluate(const EvaluationContext &context) override {
        return subClasses.count(context.feature.fSubClass) != 0;
    };
};

class SubClassNotInFilter : public Value<bool> {
private:
    const std::unordered_set<std::string> subClasses;
public:
    SubClassNotInFilter(const std::unordered_set<std::string> subClasses) : subClasses(subClasses) {}

    bool evaluate(const EvaluationContext &context) override {
        return subClasses.count(context.feature.fSubClass) == 0;
    };
};

template<typename T, typename PropertyType>
class PropertyFilter : public Value<T> {
private:
    std::shared_ptr<Value<T>> defaultValue;
    std::vector<std::pair<PropertyType, std::shared_ptr<Value<T>>>> valueMapping;

    virtual PropertyType getProperty(const EvaluationContext &context) = 0;
public:
    PropertyFilter(const std::map<std::set<PropertyType>, std::shared_ptr<Value<T>>> mapping,
                const std::shared_ptr<Value<T>> defaultValue): defaultValue(defaultValue) {
        for (auto const &entry: mapping) {
            for(auto const& v : entry.first) {
                valueMapping.emplace_back(std::make_pair(v, entry.second));
            }
        }
    }

    T evaluate(const EvaluationContext &context) override {
        const auto &p = getProperty(context);

        for(const auto& m : valueMapping) {
            if(m.first == p) {
                return m.second->evaluate(context);
            }
        }

        return defaultValue->evaluate(context);
    };
};

template<typename T>
class ClassFilter : public PropertyFilter<T, std::string> {
private:
    std::string getProperty(const EvaluationContext &context) override {
        return context.feature.fClass;
    };
public:
    using PropertyFilter<T, std::string>::PropertyFilter;
};

template<typename T>
class SubClassFilter : public PropertyFilter<T, std::string> {
private:
    std::string getProperty(const EvaluationContext &context) override {
        return context.feature.fSubClass;
    };
public:
    using PropertyFilter<T, std::string>::PropertyFilter;
};

template<typename T>
class WidthFilter : public PropertyFilter<T, uint64_t> {
private:
    uint64_t getProperty(const EvaluationContext &context) override {
        return context.feature.width;
    };
public:
    using PropertyFilter<T, uint64_t>::PropertyFilter;
};

enum class LogOpType {
    AND,
    OR,
    NOT
};

class LogOpValue : public Value<bool> {
public:
    LogOpValue(const LogOpType &logOpType, const std::shared_ptr<Value<bool>> &lhs, const std::shared_ptr<Value<bool>> &rhs = nullptr) : logOpType(logOpType), lhs(lhs), rhs(rhs) {}

    bool evaluate(const EvaluationContext &context) override {
        switch (logOpType) {
            case LogOpType::AND:
                return lhs->evaluate(context) && rhs && rhs->evaluate(context);
            case LogOpType::OR:
                return lhs->evaluate(context) || (rhs && rhs->evaluate(context));
            case LogOpType::NOT:
                return !lhs->evaluate(context);
        }
    };

private:
    const LogOpType logOpType;
    const std::shared_ptr<Value<bool>> lhs;
    const std::shared_ptr<Value<bool>> rhs;
};
