#include "DataRef.hpp"
#include "GeoJsonTypes.h"
#include "Tiled2dMapVectorLayerParserHelper.h"
#include "VectorLayerDescription.h"
#include "geojsonvt.hpp"
#include "helper/TestData.h"
#include "helper/TestLocalDataProvider.h"
#include "helper/TestScheduler.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <random>

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
    auto result = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString("test", jsonString, nullptr, {}, {});
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
    auto result = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString("test", jsonString, provider, {}, {});
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
