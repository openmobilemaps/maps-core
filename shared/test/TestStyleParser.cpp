#include "GeoJsonTypes.h"
#include "Tiled2dMapVectorLayerParserHelper.h"
#include "Tiled2dMapVectorStyleParser.h"

#include "helper/TestData.h"
#include "helper/TestLocalDataProvider.h"
#include "helper/TestScheduler.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

class TestGeoJSONTileDelegate : public GeoJSONTileDelegate, public ActorObject {
public:
    bool didLoadCalled = false;
    bool failedToLoadCalled = false;

    void didLoad(uint8_t maxZoom) override {
        didLoadCalled = true;
    }

    void failedToLoad() override {
        failedToLoadCalled = true;
    }
};


TEST_CASE("TestStyleParser", "[GeoJson inline]") {
    auto jsonString = TestData::readFileToString("style/geojson_style_inline.json");
    std::shared_ptr<StringInterner> stringTable = std::make_shared<StringInterner>(ValueKeys::newStringInterner());
    auto result = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString("test", jsonString, nullptr, {}, stringTable, {});
    REQUIRE(result.mapDescription != nullptr);
    REQUIRE(!result.mapDescription->geoJsonSources.empty());

    std::shared_ptr<GeoJSONVTInterface> geojsonSource = result.mapDescription->geoJsonSources.begin()->second;
    REQUIRE(geojsonSource->getMinZoom() == 0);
    REQUIRE(geojsonSource->getMaxZoom() == 0);

    REQUIRE(result.mapDescription->layers[0]->sourceMinZoom == 0);
    REQUIRE(result.mapDescription->layers[0]->sourceMaxZoom == 0);
    REQUIRE_NOTHROW(geojsonSource->getTile(0,0,0));
    REQUIRE_THROWS(geojsonSource->getTile(6,33,22));
}

TEST_CASE("TestStyleParser", "[GeoJson local provider]") {
    auto jsonString = TestData::readFileToString("style/geojson_style_provider.json");
    auto provider = std::make_shared<TestLocalDataProvider>(std::unordered_map<std::string, std::string>{
        {"wsource", "geojson.geojson"}
    });
    std::shared_ptr<StringInterner> stringTable = std::make_shared<StringInterner>(ValueKeys::newStringInterner());
    auto result = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString("test", jsonString, provider, {}, stringTable, {});
    REQUIRE(result.mapDescription != nullptr);
    REQUIRE(!result.mapDescription->geoJsonSources.empty());

    std::shared_ptr<GeoJSONVTInterface> geojsonSource = result.mapDescription->geoJsonSources.begin()->second;

    auto scheduler = std::make_shared<TestScheduler>();
    auto delegate = Actor<TestGeoJSONTileDelegate>(std::make_shared<Mailbox>(scheduler), std::make_shared<TestGeoJSONTileDelegate>());

    geojsonSource->setDelegate(delegate.weakActor<GeoJSONTileDelegate>());

    REQUIRE(!delegate.unsafe()->didLoadCalled);

    auto promise = std::make_shared<::djinni::Promise<std::shared_ptr<DataLoaderResult>>>();
    geojsonSource->waitIfNotLoaded(promise);
    promise->getFuture().wait();

    scheduler->drain();

    REQUIRE(delegate.unsafe()->didLoadCalled);

    REQUIRE(geojsonSource->getMinZoom() == 0);
    REQUIRE(geojsonSource->getMaxZoom() == 25);

    const auto &tile = geojsonSource->getTile(6,33,22);

    REQUIRE(!tile.getFeatures().empty());
}

TEST_CASE("String interpolation expressions") {
    struct TestCase {
        nlohmann::json expression;
        std::string expectedEvaluation;
        bool expectIsInterpolationString;
    };

    auto testCase = GENERATE(
        TestCase{
            .expression = "",
            .expectedEvaluation = "",
            .expectIsInterpolationString = false,
        },
        TestCase{
            .expression = "foo",
            .expectedEvaluation = "foo",
            .expectIsInterpolationString = false,
        },
        TestCase{
            .expression = "-{key1}.",
            .expectedEvaluation = "-value1.",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "{key1}.",
            .expectedEvaluation = "value1.",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "-{key1}",
            .expectedEvaluation = "-value1",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "{key1}",
            .expectedEvaluation = "value1",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "{ key1 }",
            .expectedEvaluation = "value1",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "{ key1 }{key2     }",
            .expectedEvaluation = "value1VALUE2",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "-{ key1 }:{key2}.",
            .expectedEvaluation = "-value1:VALUE2.",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "-{ keyDoesnaExist };",
            .expectedEvaluation = "-;",
            .expectIsInterpolationString = true,
        },
        // Special case of not existing key: empty string
        TestCase{
            .expression = "{ }",
            .expectedEvaluation = "",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "{}",
            .expectedEvaluation = "",
            .expectIsInterpolationString = true,
        },
        // XXX: Escaping does not really make sense, the backslash is not removed.
        TestCase{
            .expression = "f\\{{key1}o",
            .expectedEvaluation = "f\\{value1o",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "f\\{{ key1 }o",
            .expectedEvaluation = "f\\{value1o",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "f\\{ { key1 } \\}o",
            .expectedEvaluation = "f\\{ value1 \\}o",
            .expectIsInterpolationString = true,
        },
        TestCase{
            .expression = "fo\\{\\{o\\}o",
            .expectedEvaluation = "fo\\{\\{o\\}o",
            .expectIsInterpolationString = false,
        },
        // literal
        TestCase{
            .expression = {"literal", "{key1}"},
            .expectedEvaluation = "{key1}",
            .expectIsInterpolationString = false,
        });

    StringInterner stringTable = ValueKeys::newStringInterner();
    Tiled2dMapVectorStyleParser parser(stringTable);
    auto value = parser.parseValue(testCase.expression);

    const bool isStringInterpolationValue = (std::dynamic_pointer_cast<StringInterpolationValue>(value) != nullptr);
    REQUIRE(isStringInterpolationValue == testCase.expectIsInterpolationString);

    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT,
                                                           FeatureContext::mapType{
                                                               {stringTable.add("key1"), "value1"},
                                                               {stringTable.add("key2"), "VALUE2"},
                                                           },
                                                           0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    auto evaluated = value->evaluate(context);
    CAPTURE(testCase.expression);
    REQUIRE(std::get<std::string>(evaluated) == testCase.expectedEvaluation);
}

TEST_CASE("MaybeGet expressions") {
    struct TestCase {
        std::string name;
        nlohmann::json expression;
        ValueVariant expectedEvaluation;
    };

    auto testCase = GENERATE(
        TestCase{
            .name = "simple string should be just a literal",
            .expression = "key1",
            .expectedEvaluation = "key1",
        },
        TestCase{
            .name = "string array should be just a string array",
            .expression = {"key1", "foo"},
            .expectedEvaluation = std::vector<std::string>{"key1", "foo"},
        },
        TestCase{
            .name = "comparison operator uses maybe-get, equal",
            .expression = {"==", "key1", "value1"},
            .expectedEvaluation = true,
        },
        TestCase{
            .name = "comparison operator uses maybe-get, not equal",
            .expression = {"==", "key1", "not_value_1"},
            .expectedEvaluation = false,
        },
        TestCase{
            .name = "comparison operator uses maybe-get only for first param",
            .expression = {"==", "key1", "key1"},
            .expectedEvaluation = false,
        },
        TestCase{
            .name = "comparison operator uses maybe-get only for simple strings",
            .expression = {"==", {"literal", "key1"}, "key1"},
            .expectedEvaluation = true,
        },
        TestCase{
            .name = "comparison operator uses maybe-get, unknown key evaluates to key",
            .expression = {"==", "unknown_key", "unknown_key"},
            .expectedEvaluation = true,
        });

    StringInterner stringTable = ValueKeys::newStringInterner();
    Tiled2dMapVectorStyleParser parser(stringTable);
    auto value = parser.parseValue(testCase.expression);

    auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT,
                                                           FeatureContext::mapType{
                                                               {stringTable.add("key1"), "value1"},
                                                               {stringTable.add("key2"), "VALUE2"},
                                                           },
                                                           0);
    EvaluationContext context = EvaluationContext(0, 0, featureContext, nullptr);

    auto evaluated = value->evaluate(context);
    CAPTURE(testCase.name, testCase.expression);
    REQUIRE(evaluated == testCase.expectedEvaluation);
}

TEST_CASE("Step and zoom expressions") {
    struct TestCase {
        std::string name;
        nlohmann::json expression;
    };


    auto testCase = GENERATE(
        TestCase{
          .name = "step with [zoom]",
          .expression = {"step",
                         {"zoom"},
                         "circle_black_lt6",
                         6, "circle_black_6-8",
                         8, "circle_black_gt8"},
        },
        TestCase{
          .name = "step with [get zoom]",
          .expression = {"step",
                         {"get", "zoom"},
                         "circle_black_lt6",
                         6, "circle_black_6-8",
                         8, "circle_black_gt8"},
        },
        TestCase{
          .name = "step with [get property]",
          .expression = {"step",
                         {"get", "property1"},
                         "circle_black_lt6",
                         6, "circle_black_6-8",
                         8, "circle_black_gt8"},
        }
    );

    StringInterner stringTable = ValueKeys::newStringInterner();
    Tiled2dMapVectorStyleParser parser(stringTable);
    auto value = parser.parseValue(testCase.expression);

    std::vector<std::pair<double, std::string>> testEvals {
      {5.0, "circle_black_lt6"},
      {7.0, "circle_black_6-8"},
      {9.0, "circle_black_gt8"},
    };

    CAPTURE(testCase.name, testCase.expression);

    for(auto &[evalInput, expectedOutput] : testEvals) {
        CAPTURE(evalInput);
        auto featureContext = std::make_shared<FeatureContext>(vtzero::GeomType::POINT,
                                                               FeatureContext::mapType{
                                                                   {stringTable.add("property1"), evalInput},
                                                               },
                                                               0);
        EvaluationContext context = EvaluationContext(evalInput, 0, featureContext, nullptr);

        auto evaluated = value->evaluate(context);
        REQUIRE(std::holds_alternative<std::string>(evaluated));
        REQUIRE(std::get<std::string>(evaluated) == expectedOutput);
    }
}
