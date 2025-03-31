#include "Value.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

TEST_CASE("GetPropertyValue tests", "[GetPropertyValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    SECTION("Evaluate when key does not exist") {
        auto evaluator1 = ValueEvaluator<std::string>(std::make_shared<GetPropertyValue>("key"));
        auto result1 = evaluator1.getResult(EvaluationContext(0, 0, featureContext, featureStateManager), "fallback");
        REQUIRE(result1 == "fallback");
    }

    SECTION("Evaluate when key exist") {
        featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};
        auto staticValue = std::make_shared<GetPropertyValue>("key");
        auto evaluator = ValueEvaluator<std::string>(staticValue);
        auto result = evaluator.getResult(EvaluationContext(0, 0, featureContext, featureStateManager), "fallback");
        REQUIRE(result == "value");
    }
}

TEST_CASE("StaticValue tests", "[StaticValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

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
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    SECTION("Evaluate ToStringValue") {
        auto staticValue = std::make_shared<StaticValue>("test");
        ToStringValue value(staticValue);
        REQUIRE(std::get<std::string>(value.evaluate(context)) == "test");
    }
}

TEST_CASE("ScaleValue tests", "[ScaleValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

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
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    SECTION("Evaluate HasPropertyValue when property exists") {
        featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};
        HasPropertyValue value("key");
        REQUIRE(std::get<bool>(value.evaluate(context)) == true);
    }

    SECTION("Evaluate HasPropertyValue when property does not exist") {
        HasPropertyValue value("key");
        REQUIRE(std::get<bool>(value.evaluate(context)) == false);
    }
}

TEST_CASE("HasNotPropertyValue tests", "[HasNotPropertyValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    SECTION("Evaluate HasNotPropertyValue when property exists") {
        featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};
        HasNotPropertyValue value("key");
        REQUIRE(std::get<bool>(value.evaluate(context)) == false);
    }

    SECTION("Evaluate HasNotPropertyValue when property does not exist") {
        HasNotPropertyValue value("key");
        REQUIRE(std::get<bool>(value.evaluate(context)) == true);
    }
}

TEST_CASE("ScaleValue Test", "[ScaleValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto value = std::make_shared<StaticValue>(ValueVariant(2.0));
    ScaleValue scaleValue(value, 2.0);

    REQUIRE(std::get<double>(scaleValue.evaluate(context)) == 4.0);
}

TEST_CASE("InterpolatedValue Test", "[InterpolatedValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(5.0, 0, featureContext, featureStateManager);

    std::vector<std::pair<double, std::shared_ptr<Value>>> steps = {{0.0, std::make_shared<StaticValue>(ValueVariant(0.0))},
                                                                    {10.0, std::make_shared<StaticValue>(ValueVariant(10.0))}};

    InterpolatedValue interpolatedValue(1.0, steps);
    REQUIRE(std::get<double>(interpolatedValue.evaluate(context)) == 5.0);
}

TEST_CASE("BezierInterpolatedValue Test", "[BezierInterpolatedValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(5.0, 0, featureContext, featureStateManager);

    std::vector<std::pair<double, std::shared_ptr<Value>>> steps = {{0.0, std::make_shared<StaticValue>(ValueVariant(0.0))},
                                                                    {10.0, std::make_shared<StaticValue>(ValueVariant(10.0))}};

    BezierInterpolatedValue bezierInterpolatedValue(0.42, 0.0, 0.58, 1.0, steps);
    REQUIRE( std::get<double>(bezierInterpolatedValue.evaluate(context)) == 5.0);
}

TEST_CASE("StepValue Test", "[StepValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(5.0, 0, featureContext, featureStateManager);

    auto compareValue = std::make_shared<StaticValue>(ValueVariant(5.0));
    std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> stops = {
        {std::make_shared<StaticValue>(ValueVariant(0.0)), std::make_shared<StaticValue>(ValueVariant(0.0))},
        {std::make_shared<StaticValue>(ValueVariant(10.0)), std::make_shared<StaticValue>(ValueVariant(10.0))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant(0.0));

    StepValue stepValue(compareValue, stops, defaultValue);
    REQUIRE(std::get<double>(stepValue.evaluate(context)) == 0.0);
}

TEST_CASE("CaseValue Test", "[CaseValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};

    std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> cases = {
        {std::make_shared<HasPropertyValue>("key"), std::make_shared<StaticValue>(ValueVariant("value"))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant("default"));

    CaseValue caseValue(cases, defaultValue);
    REQUIRE(std::get<std::string>(caseValue.evaluate(context)) == "value");
}

TEST_CASE("ToNumberValue Test", "[ToNumberValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto value = std::make_shared<StaticValue>(ValueVariant("123.45"));
    ToNumberValue toNumberValue(value);

    REQUIRE(std::get<double>(toNumberValue.evaluate(context)) == 123.45);
}

TEST_CASE("ToBooleanValue Test", "[ToBooleanValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto value = std::make_shared<StaticValue>(ValueVariant("true"));
    ToBooleanValue toBooleanValue(value);

    REQUIRE(std::get<bool>(toBooleanValue.evaluate(context)) == true);
}

TEST_CASE("BooleanValue Test", "[BooleanValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto value = std::make_shared<StaticValue>(ValueVariant(true));
    BooleanValue booleanValue(value);

    REQUIRE(std::get<bool>(booleanValue.evaluate(context)) == true);
}

TEST_CASE("MatchValue Test", "[MatchValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto compareValue = std::make_shared<StaticValue>(ValueVariant("key"));
    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> mapping = {
        {ValueVariant("key"), std::make_shared<StaticValue>(ValueVariant("value"))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant("default"));

    MatchValue matchValue(compareValue, mapping, defaultValue);
    REQUIRE(std::get<std::string>(matchValue.evaluate(context)) == "value");
}

TEST_CASE("PropertyFilter Test", "[PropertyFilter]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};

    std::vector<std::pair<ValueVariant, std::shared_ptr<Value>>> mapping = {
        {ValueVariant("value"), std::make_shared<StaticValue>(ValueVariant("matched"))}};
    auto defaultValue = std::make_shared<StaticValue>(ValueVariant("default"));

    PropertyFilter propertyFilter(mapping, defaultValue, "key");
    REQUIRE(std::get<std::string>(propertyFilter.evaluate(context)) == "matched");
}

TEST_CASE("LogOpValue Test", "[LogOpValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto lhs = std::make_shared<StaticValue>(ValueVariant(true));
    auto rhs = std::make_shared<StaticValue>(ValueVariant(false));

    LogOpValue logOpValue(LogOpType::AND, lhs, rhs);
    REQUIRE(std::get<bool>(logOpValue.evaluate(context)) == false);
}

TEST_CASE("AllValue Test", "[AllValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant(true)),
                                                  std::make_shared<StaticValue>(ValueVariant(true))};

    AllValue allValue(values);
    REQUIRE(std::get<bool>(allValue.evaluate(context)) == true);
}

TEST_CASE("AnyValue Test", "[AnyValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant(false)),
                                                  std::make_shared<StaticValue>(ValueVariant(true))};

    AnyValue anyValue(values);
    REQUIRE(std::get<bool>(anyValue.evaluate(context)) == true);
}

TEST_CASE("PropertyCompareValue Test", "[PropertyCompareValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto lhs = std::make_shared<StaticValue>(ValueVariant(5.0));
    auto rhs = std::make_shared<StaticValue>(ValueVariant(10.0));

    PropertyCompareValue propertyCompareValue(lhs, rhs, PropertyCompareType::LESS);
    REQUIRE(std::get<bool>(propertyCompareValue.evaluate(context)) == true);
}

TEST_CASE("InFilter Test", "[InFilter]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};

    std::unordered_set<ValueVariant> values = {ValueVariant("value")};
    auto dynamicValues = std::make_shared<StaticValue>(ValueVariant(std::vector<std::string>{"value"}));

    InFilter inFilter("key", values, dynamicValues);
    REQUIRE(std::get<bool>(inFilter.evaluate(context)) == true);
}

TEST_CASE("NotInFilter Test", "[NotInFilter]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    featureContext->propertiesMap = FeatureContext::mapType{{"key", "value"}};

    std::unordered_set<ValueVariant> values = {ValueVariant("other")};
    auto dynamicValues = std::make_shared<StaticValue>(ValueVariant(std::vector<std::string>{"other"}));

    NotInFilter notInFilter("key", values, dynamicValues);
    REQUIRE(std::get<bool>(notInFilter.evaluate(context)) == true);
}

TEST_CASE("FormatValue Test", "[FormatValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    std::vector<FormatValueWrapper> values = {{std::make_shared<StaticValue>(ValueVariant("test")), 1.0f}};

    FormatValue formatValue(values);
    auto result = std::get<std::vector<FormattedStringEntry>>(formatValue.evaluate(context));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].text == "test");
    REQUIRE(result[0].scale == 1.0f);
}

TEST_CASE("NumberFormatValue Test", "[NumberFormatValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto value = std::make_shared<StaticValue>(ValueVariant(123.456));
    NumberFormatValue numberFormatValue(value, 2, 2);

    REQUIRE(std::get<std::string>(numberFormatValue.evaluate(context)) == "123.46");
}

TEST_CASE("MathValue Test", "[MathValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto lhs = std::make_shared<StaticValue>(ValueVariant(5.0));
    auto rhs = std::make_shared<StaticValue>(ValueVariant(3.0));

    MathValue mathValue(lhs, rhs, MathOperation::PLUS);
    REQUIRE(std::get<double>(mathValue.evaluate(context)) == 8.0);
}

TEST_CASE("LengthValue Test", "[LengthValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    auto value = std::make_shared<StaticValue>(ValueVariant("test"));
    LengthValue lengthValue(value);

    REQUIRE(std::get<int64_t>(lengthValue.evaluate(context)) == 4);
}

TEST_CASE("CoalesceValue Test", "[CoalesceValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant(std::monostate())),
                                                  std::make_shared<StaticValue>(ValueVariant("value"))};

    CoalesceValue coalesceValue(values);
    REQUIRE(std::get<std::string>(coalesceValue.evaluate(context)) == "value");
}

TEST_CASE("ArrayValue Test", "[ArrayValue]") {
    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto featureStateManager = std::make_shared<Tiled2dMapVectorStateManager>();
    EvaluationContext context = EvaluationContext(0, 0, featureContext, featureStateManager);

    std::vector<std::shared_ptr<Value>> values = {std::make_shared<StaticValue>(ValueVariant("value1")),
                                                  std::make_shared<StaticValue>(ValueVariant("value2"))};

    ArrayValue arrayValue(values);
    auto result = std::get<std::vector<std::string>>(arrayValue.evaluate(context));
    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == "value1");
    REQUIRE(result[1] == "value2");
}