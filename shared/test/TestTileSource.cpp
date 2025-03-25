#include "CoordinateSystemFactory.h"
#include "DefaultSystemToRenderConverter.h"
#include "SymbolAnimationCoordinatorMap.h"
#include "Tiled2dMapSource.h"
#include "VectorTileGeometryHandler.h"
#include "WebMercatorTiled2dMapLayerConfig.h"
#include "helper/GeoJsonGenerator.h"

#include "helper/TestScheduler.h"
#include <algorithm>
#include <atomic>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

struct TestLoaderResult final {
    std::optional<std::string> etag = std::nullopt;
    LoaderStatus status = LoaderStatus::OK;
    std::optional<std::string> errorCode = std::nullopt;
};

class TestTiled2dMapVectorSource : public Tiled2dMapSource<int, std::shared_ptr<TestLoaderResult>, int> {
  public:
    TestTiled2dMapVectorSource(const std::shared_ptr<TestScheduler> scheduler)
        : Tiled2dMapSource(MapConfig(CoordinateSystemFactory::getEpsg3857System()),
                           std::make_shared<WebMercatorTiled2dMapLayerConfig>(
                               "mock", "{z}/{x}/{y}", Tiled2dMapZoomInfo(1.0, 0, 0, false, true, false, true), 0, 20),
                           CoordinateConversionHelperInterface::independentInstance(), scheduler, 62, 1, "layer") {};

    std::unordered_set<Tiled2dMapTileInfo> getCurrentTiles() {
        std::unordered_set<Tiled2dMapTileInfo> tiles;
        for (const auto &tile : currentTiles) {
            tiles.insert(tile.first);
        }
        return tiles;
    }

    void notifyTilesUpdates() override {}

  protected:
    void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override {}

    ::djinni::Future<std::shared_ptr<TestLoaderResult>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override {
        auto promise = std::make_shared<::djinni::Promise<std::shared_ptr<TestLoaderResult>>>();
        promise->setValue(std::make_shared<TestLoaderResult>());
        return promise->getFuture();
    }

    bool hasExpensivePostLoadingTask() override { return false; }

    int postLoadingTask(std::shared_ptr<TestLoaderResult> loadedData, Tiled2dMapTileInfo tile) override { return 12; }
};

TEST_CASE("VectorTileSource") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::shared_ptr<TestTiled2dMapVectorSource> source = std::make_shared<TestTiled2dMapVectorSource>(scheduler);
    source->mailbox = std::make_shared<Mailbox>(scheduler);

    SECTION("basic rect") {
        auto rect = RectCoord(Coord(3857, 409758, 6263475, 0), Coord(3857, 1390769, 5579492, 0));
        source->onVisibleBoundsChanged(rect, 0, 4371851);
        scheduler->drain();

        auto tiles = source->getCurrentTiles();

        // ignore the rect for assertions
        RectCoord empty = rect;
        std::vector<Tiled2dMapTileInfo> expectedTiles = {{empty, 0, 0, 0, 0, 559082264}, {empty, 32, 22, 0, 6, 8735660},
                                                         {empty, 33, 21, 0, 6, 8735660}, {empty, 34, 23, 0, 6, 8735660},
                                                         {empty, 32, 21, 0, 6, 8735660}, {empty, 33, 23, 0, 6, 8735660},
                                                         {empty, 32, 23, 0, 6, 8735660}, {empty, 33, 22, 0, 6, 8735660},
                                                         {empty, 34, 21, 0, 6, 8735660}, {empty, 34, 22, 0, 6, 8735660}};

        REQUIRE(tiles.size() == 10);
        for (const auto &expectedTile : expectedTiles) {
            REQUIRE(std::find_if(tiles.begin(), tiles.end(), [&expectedTile](const auto &tile) {
                        return tile.x == expectedTile.x && tile.y == expectedTile.y &&
                               tile.zoomIdentifier == expectedTile.zoomIdentifier && tile.zoomLevel == expectedTile.zoomLevel;
                    }) != tiles.end());
        }

        // for debugging
        // GeoJsonGenerator generator;
        // generator.addRect(rect, "#0000FF");
        // for (const auto& tile : tiles) {
        //     generator.addRect(tile.bounds, "#FF0000");
        // }
        // generator.printGeoJson();
    };
}