#include "Value.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

TEST_CASE("GetPropertyValue tests", "[GetPropertyValue]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto key = stringTable.add("key");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    SECTION("Evaluate when key does not exist") {
        GetPropertyValue value = GetPropertyValue(key);
        auto result = value.evaluateOr<std::string>(context, "fallback");
        REQUIRE(result == "fallback");
    }

    SECTION("Evaluate when key exist") {
        featureContext->propertiesMap = FeatureContext::mapType{{key, "value"}};
        GetPropertyValue value = GetPropertyValue(key);
        auto result = std::get<std::string>(value.evaluate(context));
        REQUIRE(result == "value");
    }
}

TEST_CASE("StaticValue tests", "[StaticValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    SECTION("Evaluate string value") {
        StaticValue value("test");
        REQUIRE(std::get<std::string>(value.evaluate(context)) == "test");
    }

    SECTION("Evaluate double value") {
        StaticValue value(3.14);
        REQUIRE(std::get<double>(value.evaluate(context)) == 3.14);
    }

    SECTION("Evaluate int64_t value") {
        StaticValue value(int64_t(42));
        REQUIRE(std::get<int64_t>(value.evaluate(context)) == 42);
    }
}

TEST_CASE("ToStringValue tests", "[ToStringValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    SECTION("Evaluate ToStringValue") {
        auto staticValue = std::make_shared<StaticValue>("test");
        ToStringValue value(staticValue);
        REQUIRE(std::get<std::string>(value.evaluate(context)) == "test");
    }
}

TEST_CASE("ScaleValue tests", "[ScaleValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    SECTION("Evaluate ScaleValue with double") {
        auto staticValue = std::make_shared<StaticValue>(2.0);
        ScaleValue value(staticValue, 3.0);
        REQUIRE(std::get<double>(value.evaluate(context)) == 6.0);
    }

    SECTION("Evaluate ScaleValue with int64_t") {
        auto staticValue = std::make_shared<StaticValue>(int64_t(2));
        ScaleValue value(staticValue, 3.0);
        REQUIRE(std::get<double>(value.evaluate(context)) == 6.0);
    }
}

TEST_CASE("HasPropertyValue tests", "[HasPropertyValue]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto key = stringTable.add("key");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    SECTION("Evaluate HasPropertyValue when property exists") {
        featureContext->propertiesMap = FeatureContext::mapType{{key, "value"}};
        HasPropertyValue value(key);
        REQUIRE(std::get<bool>(value.evaluate(context)) == true);
    }

    SECTION("Evaluate HasPropertyValue when property does not exist") {
        HasPropertyValue value(key);
        REQUIRE(std::get<bool>(value.evaluate(context)) == false);
    }
}

TEST_CASE("HasNotPropertyValue tests", "[HasNotPropertyValue]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto key = stringTable.add("key");
      auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    SECTION("Evaluate HasNotPropertyValue when property exists") {
        featureContext->propertiesMap = FeatureContext::mapType{{key, "value"}};
        HasNotPropertyValue value(key);
        REQUIRE(std::get<bool>(value.evaluate(context)) == false);
    }

    SECTION("Evaluate HasNotPropertyValue when property does not exist") {
        HasNotPropertyValue value(key);
        REQUIRE(std::get<bool>(value.evaluate(context)) == true);
    }
}

TEST_CASE("InterpolatedValue Test", "[InterpolatedValue]") {
    EvaluationContext context = EvaluationContext(5.0, 0, nullptr, nullptr);

    std::vector<std::pair<double, std::shared_ptr<Value>>> steps = {{0.0, std::make_shared<StaticValue>(ValueVariant(0.0))},
                                                                    {10.0, std::make_shared<StaticValue>(ValueVariant(10.0))}};

    InterpolatedValue interpolatedValue(1.0, steps);
    REQUIRE(std::get<double>(interpolatedValue.evaluate(context)) == 5.0);
}

TEST_CASE("BezierInterpolatedValue Test", "[BezierInterpolatedValue]") {
    EvaluationContext context = EvaluationContext(5.0, 0, nullptr, nullptr);

    std::vector<std::pair<double, std::shared_ptr<Value>>> steps = {{0.0, std::make_shared<StaticValue>(ValueVariant(0.0))},
                                                                    {10.0, std::make_shared<StaticValue>(ValueVariant(10.0))}};

    BezierInterpolatedValue bezierInterpolatedValue(0.42, 0.0, 0.58, 1.0, steps);
    REQUIRE( std::get<double>(bezierInterpolatedValue.evaluate(context)) == 5.0);
}

TEST_CASE("StepValue Test", "[StepValue]") {
    EvaluationContext context = EvaluationContext(5.0, 0, nullptr, nullptr);

    auto compareValue = std::make_shared<StaticValue>(ValueVariant(5.0));
    std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops = {
        {std::make_shared<StaticValue>(ValueVariant(0.0)), std::make_shared<StaticValue>(ValueVariant(0.0))},
        {std::make_shared<StaticValue>(ValueVariant(10.0)), std::make_shared<StaticValue>(ValueVariant(10.0))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant(0.0));

    StepValue stepValue(compareValue, stops, defaultValue);
    REQUIRE(std::get<double>(stepValue.evaluate(context)) == 0.0);
}

TEST_CASE("CaseValue Test", "[CaseValue]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto key = stringTable.add("key");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{{key, "value"}}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases = {
        {std::make_shared<HasPropertyValue>(key), std::make_shared<StaticValue>(ValueVariant("value"))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant("default"));

    CaseValue caseValue(cases, defaultValue);
    REQUIRE(std::get<std::string>(caseValue.evaluate(context)) == "value");
}

TEST_CASE("ToNumberValue Test", "[ToNumberValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto value = std::make_shared<StaticValue>(ValueVariant("123.45"));
    ToNumberValue toNumberValue(value);

    REQUIRE(std::get<double>(toNumberValue.evaluate(context)) == 123.45);
}

TEST_CASE("ToBooleanValue Test", "[ToBooleanValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto value = std::make_shared<StaticValue>(ValueVariant("true"));
    ToBooleanValue toBooleanValue(value);

    REQUIRE(std::get<bool>(toBooleanValue.evaluate(context)) == true);
}

TEST_CASE("BooleanValue Test", "[BooleanValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto value = std::make_shared<StaticValue>(ValueVariant(true));
    BooleanValue booleanValue(value);

    REQUIRE(std::get<bool>(booleanValue.evaluate(context)) == true);
}

TEST_CASE("MatchValue Test", "[MatchValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto compareValue = std::make_shared<StaticValue>(ValueVariant("key"));
    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> mapping = {
        {ValueVariant("key"), std::make_shared<StaticValue>(ValueVariant("value"))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant("default"));

    MatchValue matchValue(compareValue, mapping, defaultValue);
    REQUIRE(std::get<std::string>(matchValue.evaluate(context)) == "value");
}

TEST_CASE("PropertyFilter Test", "[PropertyFilter]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto key = stringTable.add("key");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{{key, "value"}}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> mapping = {
        {ValueVariant("value"), std::make_shared<StaticValue>(ValueVariant("matched"))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant("default"));

    PropertyFilter propertyFilter(mapping, defaultValue, key);
    REQUIRE(std::get<std::string>(propertyFilter.evaluate(context)) == "matched");
}

TEST_CASE("LogOpValue Test", "[LogOpValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto lhs = std::make_shared<StaticValue>(ValueVariant(true));
    auto rhs = std::make_shared<StaticValue>(ValueVariant(false));

    LogOpValue logOpValue(LogOpType::AND, lhs, rhs);
    REQUIRE(std::get<bool>(logOpValue.evaluate(context)) == false);
}

TEST_CASE("AllValue Test", "[AllValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant(true)),
                                                  std::make_shared<StaticValue>(ValueVariant(true))};

    AllValue allValue(values);
    REQUIRE(std::get<bool>(allValue.evaluate(context)) == true);
}

TEST_CASE("AnyValue Test", "[AnyValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant(false)),
                                                  std::make_shared<StaticValue>(ValueVariant(true))};

    AnyValue anyValue(values);
    REQUIRE(std::get<bool>(anyValue.evaluate(context)) == true);
}

TEST_CASE("PropertyCompareValue Test", "[PropertyCompareValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto lhs = std::make_shared<StaticValue>(ValueVariant(5.0));
    auto rhs = std::make_shared<StaticValue>(ValueVariant(10.0));

    PropertyCompareValue propertyCompareValue(lhs, rhs, PropertyCompareType::LESS);
    REQUIRE(std::get<bool>(propertyCompareValue.evaluate(context)) == true);
}

TEST_CASE("InFilter Test", "[InFilter]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto internedStringKey = stringTable.add("key");
    auto key = std::make_shared<MaybeGetPropertyValue>(internedStringKey, "key");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{{internedStringKey, "value"}}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    std::unordered_set<ValueVariant> values = {ValueVariant("value")};
    auto dynamicValues = std::make_shared<StaticValue>(ValueVariant(std::vector<std::string>{"value"}));

    InFilter inFilter(key, values, dynamicValues);
    REQUIRE(std::get<bool>(inFilter.evaluate(context)) == true);
}

TEST_CASE("NotInFilter Test", "[NotInFilter]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto internedStringKey = stringTable.add("key");
    auto key = std::make_shared<MaybeGetPropertyValue>(internedStringKey, "key");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{{internedStringKey, "value"}}, 0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    std::unordered_set<ValueVariant> values = {ValueVariant("other")};
    auto dynamicValues = std::make_shared<StaticValue>(ValueVariant(std::vector<std::string>{"other"}));

    NotInFilter notInFilter(key, values, dynamicValues);
    REQUIRE(std::get<bool>(notInFilter.evaluate(context)) == true);
}

TEST_CASE("FormatValue Test", "[FormatValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    std::vector<FormatValueWrapper> values = {{std::make_shared<StaticValue>(ValueVariant("test")), 1.0f}};

    FormatValue formatValue(values);
    auto result = std::get<std::vector<FormattedStringEntry>>(formatValue.evaluate(context));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].text == "test");
    REQUIRE(result[0].scale == 1.0f);
}

TEST_CASE("NumberFormatValue Test", "[NumberFormatValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto value = std::make_shared<StaticValue>(ValueVariant(123.456));
    NumberFormatValue numberFormatValue(value, 2, 2);

    REQUIRE(std::get<std::string>(numberFormatValue.evaluate(context)) == "123.46");
}

TEST_CASE("MathValue Test", "[MathValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto lhs = std::make_shared<StaticValue>(ValueVariant(5.0));
    auto rhs = std::make_shared<StaticValue>(ValueVariant(3.0));

    MathValue mathValue(lhs, rhs, MathOperation::PLUS);
    REQUIRE(std::get<double>(mathValue.evaluate(context)) == 8.0);
}

TEST_CASE("LengthValue Test", "[LengthValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    auto value = std::make_shared<StaticValue>(ValueVariant("test"));
    LengthValue lengthValue(value);

    REQUIRE(std::get<int64_t>(lengthValue.evaluate(context)) == 4);
}

TEST_CASE("CoalesceValue Test", "[CoalesceValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant(std::monostate())),
                                                  std::make_shared<StaticValue>(ValueVariant("value"))};

    CoalesceValue coalesceValue(values);
    REQUIRE(std::get<std::string>(coalesceValue.evaluate(context)) == "value");
}

TEST_CASE("ArrayValue Test", "[ArrayValue]") {
    EvaluationContext context = EvaluationContext(0, 0, nullptr, nullptr);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant("value1")),
                                                  std::make_shared<StaticValue>(ValueVariant("value2"))};

    ArrayValue arrayValue(values);
    auto result = std::get<std::vector<std::string>>(arrayValue.evaluate(context));
    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == "value1");
    REQUIRE(result[1] == "value2");
}

namespace {
std::shared_ptr<MatchValue> makePropertyMatch(InternedString propertyKey,
                                              const std::vector<std::pair<std::string, double>> &branches,
                                              double defaultValue) {
    auto compareValue = std::make_shared<GetPropertyValue>(propertyKey);
    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> mapping;
    mapping.reserve(branches.size());
    for (const auto &[branchKey, branchValue] : branches) {
        mapping.emplace_back(branchKey, std::make_shared<StaticValue>(branchValue));
    }
    return std::make_shared<MatchValue>(compareValue, mapping, std::make_shared<StaticValue>(defaultValue));
}
} // namespace

TEST_CASE("InterpolatedTransposedMatchValue Test", "[InterpolatedTransposedMatchValue]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto subclassKey = stringTable.add("subclass");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    EvaluationContext context(11.0, 0, featureContext, nullptr);

    const auto grassMatch10 = makePropertyMatch(subclassKey, { { "grass", 1.0 } }, 0.0);
    const auto grassMatch12 = makePropertyMatch(subclassKey, { { "grass", 3.0 }, { "wood", 5.0 } }, 0.0);

    std::vector<std::pair<double, std::shared_ptr<Value>>> steps = {
        { 10.0, grassMatch10 },
        { 12.0, grassMatch12 },
    };

    const auto value = InterpolatedTransposedMatchValue::tryCreate(1.0, steps);
    REQUIRE(value != nullptr);

    SECTION("Known subclass interpolates branch values") {
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(context)), Catch::Matchers::WithinAbs(2.0, 1e-9));
    }

    SECTION("Subclass missing from one match stop uses that stop's default") {
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("wood") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(context)), Catch::Matchers::WithinAbs(2.5, 1e-9));
    }

    SECTION("Unmentioned subclass uses default interpolator") {
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("sand") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(context)), Catch::Matchers::WithinAbs(0.0, 1e-9));
    }

    SECTION("Mixed literal and match stops apply literal to all subclasses") {
        std::vector<std::pair<double, std::shared_ptr<Value>>> mixedSteps = {
            { 10.0, grassMatch10 },
            { 12.0, std::make_shared<StaticValue>(10.0) },
        };

        const auto mixedValue = InterpolatedTransposedMatchValue::tryCreate(1.0, mixedSteps);
        REQUIRE(mixedValue != nullptr);

        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(mixedValue->evaluate(context)), Catch::Matchers::WithinAbs(5.5, 1e-9));

        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("sand") } };
        REQUIRE_THAT(std::get<double>(mixedValue->evaluate(context)), Catch::Matchers::WithinAbs(5.0, 1e-9));
    }

    SECTION("All literal stops do not use transposed fast path") {
        std::vector<std::pair<double, std::shared_ptr<Value>>> literalSteps = {
            { 10.0, std::make_shared<StaticValue>(1.0) },
            { 12.0, std::make_shared<StaticValue>(3.0) },
        };
        REQUIRE(InterpolatedTransposedMatchValue::tryCreate(1.0, literalSteps) == nullptr);
    }

    SECTION("isEqual compares branch keys and interpolators") {
        const auto grassMatch10Other = makePropertyMatch(subclassKey, { { "grass", 9.0 } }, 0.0);
        const auto otherValue = InterpolatedTransposedMatchValue::tryCreate(1.0, {
            { 10.0, grassMatch10Other },
            { 12.0, grassMatch12 },
        });
        REQUIRE(otherValue != nullptr);
        REQUIRE_FALSE(value->isEqual(otherValue));
        REQUIRE(value->isEqual(value));
    }

    SECTION("clone preserves evaluation") {
        const auto cloned = std::shared_ptr<Value>(value->clone());
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(cloned->evaluate(context)), Catch::Matchers::WithinAbs(2.0, 1e-9));
        REQUIRE(value->isEqual(cloned));
    }
}

TEST_CASE("StepTransposedMatchValue Test", "[StepTransposedMatchValue]") {
    StringInterner stringTable = ValueKeys::newStringInterner();
    auto subclassKey = stringTable.add("subclass");
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    EvaluationContext context(11.0, 0, featureContext, nullptr);

    const auto defaultMatch = makePropertyMatch(subclassKey, {}, 0.0);
    const auto grassMatch10 = makePropertyMatch(subclassKey, { { "grass", 1.0 } }, 0.0);
    const auto grassMatch12 = makePropertyMatch(subclassKey, { { "grass", 3.0 }, { "wood", 5.0 } }, 0.0);

    std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops = {
        { std::make_shared<StaticValue>(10.0), grassMatch10 },
        { std::make_shared<StaticValue>(12.0), grassMatch12 },
    };

    const auto value = StepTransposedMatchValue::tryCreate(std::make_shared<ZoomValue>(), defaultMatch, stops);
    REQUIRE(value != nullptr);

    SECTION("Known subclass uses stepped branch values") {
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(context)), Catch::Matchers::WithinAbs(1.0, 1e-9));
    }

    SECTION("Subclass missing from one match stop uses that stop's default") {
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("wood") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(context)), Catch::Matchers::WithinAbs(0.0, 1e-9));
    }

    SECTION("Unmentioned subclass uses default stepper") {
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("sand") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(context)), Catch::Matchers::WithinAbs(0.0, 1e-9));
    }

    SECTION("Higher zoom uses later stop") {
        EvaluationContext highZoomContext(13.0, 0, featureContext, nullptr);
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(highZoomContext)), Catch::Matchers::WithinAbs(3.0, 1e-9));

        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("wood") } };
        REQUIRE_THAT(std::get<double>(value->evaluate(highZoomContext)), Catch::Matchers::WithinAbs(5.0, 1e-9));
    }

    SECTION("Mixed literal and match stops apply literal to all subclasses") {
        std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> mixedStops = {
            { std::make_shared<StaticValue>(10.0), grassMatch10 },
            { std::make_shared<StaticValue>(12.0), std::make_shared<StaticValue>(10.0) },
        };

        const auto mixedValue = StepTransposedMatchValue::tryCreate(std::make_shared<ZoomValue>(),
                                                                    std::make_shared<StaticValue>(0.0),
                                                                    mixedStops);
        REQUIRE(mixedValue != nullptr);

        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(mixedValue->evaluate(context)), Catch::Matchers::WithinAbs(1.0, 1e-9));

        EvaluationContext highZoomContext(13.0, 0, featureContext, nullptr);
        REQUIRE_THAT(std::get<double>(mixedValue->evaluate(highZoomContext)), Catch::Matchers::WithinAbs(10.0, 1e-9));
    }

    SECTION("isEqual compares branch keys and steppers") {
        const auto grassMatch10Other = makePropertyMatch(subclassKey, { { "grass", 9.0 } }, 0.0);
        const auto otherValue = StepTransposedMatchValue::tryCreate(std::make_shared<ZoomValue>(), defaultMatch, {
            { std::make_shared<StaticValue>(10.0), grassMatch10Other },
            { std::make_shared<StaticValue>(12.0), grassMatch12 },
        });
        REQUIRE(otherValue != nullptr);
        REQUIRE_FALSE(value->isEqual(otherValue));
        REQUIRE(value->isEqual(value));
    }

    SECTION("clone preserves evaluation") {
        const auto cloned = std::shared_ptr<Value>(value->clone());
        featureContext->propertiesMap = FeatureContext::mapType{ { subclassKey, std::string("grass") } };
        REQUIRE_THAT(std::get<double>(cloned->evaluate(context)), Catch::Matchers::WithinAbs(1.0, 1e-9));
        REQUIRE(value->isEqual(cloned));
    }
}
