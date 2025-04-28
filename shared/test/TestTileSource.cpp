#include "CoordinateSystemFactory.h"
#include "DataLoaderResult.h"
#include "LoaderInterface.h"
#include "TextureLoaderResult.h"
#include "Tiled2dMapSource.h"
#include "WebMercatorTiled2dMapLayerConfig.h"
#include "helper/TestScheduler.h"

#include "Tiled2dMapSourceImpl.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <cassert>
#include <cstdlib>
#include <list>
#include <memory>
#include <random>
#include <string>
#include <vector>

class TestTiled2dMapVectorSource : public Tiled2dMapSource<std::shared_ptr<DataLoaderResult>, std::string> {
  public:
    TestTiled2dMapVectorSource(std::shared_ptr<Tiled2dMapLayerConfig> layerConfig, std::shared_ptr<TestScheduler> scheduler,
                               std::vector<std::shared_ptr<LoaderInterface>> loaders)
        : Tiled2dMapSource(MapConfig(CoordinateSystemFactory::getEpsg3857System()), layerConfig,
                           CoordinateConversionHelperInterface::independentInstance(), scheduler, 62, loaders.size(), "layer")
        , layerConfig(layerConfig)
        , loaders(loaders) {}

    std::unordered_set<Tiled2dMapTileInfo> getCurrentTiles() const {
        std::unordered_set<Tiled2dMapTileInfo> tiles;
        for (const auto &tile : currentTiles) {
            tiles.insert(tile.first);
        }
        return tiles;
    }

    std::unordered_map<std::string, std::string> getCurrentTileUrlsAndData() const {
        std::unordered_map<std::string, std::string> tiles;
        for (const auto &tile : currentTiles) {
            tiles.emplace(layerConfig->getTileUrl(tile.first.x, tile.first.y, tile.first.t, tile.first.zoomIdentifier),
                          tile.second.result);
        }
        return tiles;
    }

    size_t numLoadingOrQueued() const {
        size_t n = currentlyLoading.size();
        for (const auto &queue : loadingQueues) {
            n += queue.size();
        }
        return n;
    }

    int64_t waitTimeAfterRetry(int retry) const {
        return std::min((1 << retry) * MIN_WAIT_TIME, MAX_WAIT_TIME);
    }

    std::unordered_map<size_t, std::map<Tiled2dMapTileInfo, ErrorInfo>> getErrorTiles() const {
        return errorTiles;
    }

    void notifyTilesUpdates() override {}

  protected:
    void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override {
        loaders[loaderIndex]->cancel(layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier));
    }

    ::djinni::Future<std::shared_ptr<DataLoaderResult>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override {
        auto url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
        return loaders[loaderIndex]
            ->loadDataAsync(std::move(url), std::nullopt)
            .then([](djinni::Future<DataLoaderResult> r) -> std::shared_ptr<DataLoaderResult> {
                return std::make_shared<DataLoaderResult>(r.get());
            });
    }

    bool hasExpensivePostLoadingTask() override { return false; }

    std::string postLoadingTask(std::shared_ptr<DataLoaderResult> loadedData, Tiled2dMapTileInfo tile) override {
        if (!loadedData->data.has_value()) {
            return std::string{};
        }
        return std::string((const char *)loadedData->data->buf(), loadedData->data->len());
    }

  private:
    std::vector<std::shared_ptr<LoaderInterface>> loaders;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
};

// For tests where we dont look at the loaded data, returns no data, successfully, immediately.
class NothingTestLoader : public LoaderInterface {
  public:
    virtual TextureLoaderResult loadTexture(const std::string &url, const std::optional<std::string> &etag) override {
        assert(false);
        std::abort();
    }

    virtual ::djinni::Future<TextureLoaderResult> loadTextureAsync(const std::string &url,
                                                                   const std::optional<std::string> &etag) override {
        assert(false);
        std::abort();
    }

    virtual DataLoaderResult loadData(const std::string &url, const std::optional<std::string> &etag) override {
        return loadDataAsync(url, etag).get();
    }

    virtual ::djinni::Future<DataLoaderResult> loadDataAsync(const std::string &url,
                                                             const std::optional<std::string> &etag) override {
        auto promise = ::djinni::Promise<DataLoaderResult>();
        promise.setValue(DataLoaderResult{std::nullopt, std::nullopt, LoaderStatus::OK, std::nullopt});
        return promise.getFuture();
    }

    virtual void cancel(const std::string &url) override {
        assert(false);
        std::abort();
    }
};

class BlockingTestLoader : public LoaderInterface {
  public:
    // data: URL -> data string
    BlockingTestLoader(std::unordered_map<std::string, std::string> data, float errorRate = 0.f)
        : data(std::move(data))
        , errorRnd(Catch::getSeed())
        , errorRate(errorRate)
    {}

    virtual TextureLoaderResult loadTexture(const std::string &url, const std::optional<std::string> &etag) override {
        assert(false);
        std::abort();
    }

    virtual ::djinni::Future<TextureLoaderResult> loadTextureAsync(const std::string &url,
                                                                   const std::optional<std::string> &etag) override {
        assert(false);
        std::abort();
    }

    virtual DataLoaderResult loadData(const std::string &url, const std::optional<std::string> &etag) override {
        return loadDataAsync(url, etag).get();
    }

    virtual ::djinni::Future<DataLoaderResult> loadDataAsync(const std::string &url,
                                                             const std::optional<std::string> &etag) override {
        std::unique_lock lock(mutex);
        auto &load = blockedLoads.emplace_back(url, djinni::Promise<DataLoaderResult>());
        return load.promise.getFuture();
    }

    virtual void cancel(const std::string &url) override {
        assert(false);
        std::abort();
    }

  public:
    bool unblockFirst() {
        std::unique_lock lock(mutex);
        if (blockedLoads.empty()) {
            return false;
        }
        auto load = std::move(blockedLoads.front());
        blockedLoads.pop_front();
        lock.unlock();

        unblock(std::move(load));
        return true;
    }

    bool unblockAll() {
        if (!unblockFirst()) {
            return false;
        }
        while (unblockFirst()) {
            // keep at it
        }
        return true;
    }

    void setErrorRate(float errorRate_) {
        errorRate = errorRate_;
    }

  private:
    struct BlockedLoad {
        std::string url;
        djinni::Promise<DataLoaderResult> promise;
    };

  private:
    void unblock(BlockedLoad load) {
        auto it = data.find(load.url);
        if (it != data.end()) {
            if (errorRate == 0.f || std::uniform_real_distribution<float>(0.f, 1.f)(errorRnd) > errorRate) {
                load.promise.setValue(DataLoaderResult{djinni::DataRef(it->second), std::nullopt, LoaderStatus::OK, std::nullopt});
            } else {
                load.promise.setValue(
                    DataLoaderResult{djinni::DataRef(it->second), std::nullopt, LoaderStatus::ERROR_OTHER, "random error"});
            }
        } else {
            load.promise.setValue(DataLoaderResult{std::nullopt, std::nullopt, LoaderStatus::NOOP, std::nullopt});
        }
    }

  private:
    const std::unordered_map<std::string, std::string> data;
    float errorRate;
    std::default_random_engine errorRnd;

    std::mutex mutex;
    std::list<BlockedLoad> blockedLoads;
};

TEST_CASE("VectorTileSource") {
    auto layerConfig = std::make_shared<WebMercatorTiled2dMapLayerConfig>(
        "mock", "{z}/{x}/{y}", Tiled2dMapZoomInfo(1.0, 0, 0, false, true, false, true), 0, 20);
    auto scheduler = std::make_shared<TestScheduler>();
    std::shared_ptr<TestTiled2dMapVectorSource> source = std::make_shared<TestTiled2dMapVectorSource>(
        layerConfig, scheduler, std::vector<std::shared_ptr<LoaderInterface>>{std::make_shared<NothingTestLoader>()});
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

        REQUIRE_THAT(tiles, Catch::Matchers::UnorderedRangeEquals(expectedTiles));

        // for debugging
        // GeoJsonGenerator generator;
        // generator.addRect(rect, "#0000FF");
        // for (const auto& tile : tiles) {
        //     generator.addRect(tile.bounds, "#FF0000");
        // }
        // generator.printGeoJson();
    };
}

static std::unordered_map<std::string, std::string> generateDummyData(const std::vector<Tiled2dMapTileInfo> &tiles,
                                                                      Tiled2dMapLayerConfig &layerConfig, std::string dataPrefix) {
    std::unordered_map<std::string, std::string> data;
    for (auto &tile : tiles) {
        std::string url = layerConfig.getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
        data.emplace(url, dataPrefix + url);
    }
    return data;
}

/**
 * Test that loading tiles from a fallback "remote" loader does not block a primary "local" loader.
 * "local" loads here are simmulated to return immediately, while "remote"
 * loads are slower than -- the first remote load returns after the last local
 * load.
 * This test is repeated for different number of total tiles (zoom level) and fraction of local tiles, via Catch GENERATE.
 */
TEST_CASE("Tiled2dMapSource slow fallback does not block local loads") {

    auto layerConfig = std::make_shared<WebMercatorTiled2dMapLayerConfig>(
        "mock", "test-data://tile/{z}/{x}/{y}", Tiled2dMapZoomInfo(1.0, 0, 0, false, true, false, true), 0, 20);

    // Load the entire world.
    auto rect = *layerConfig->getBounds();
    auto zoomLevelInfos = layerConfig->getZoomLevelInfos();
    // Try with different zoom levels; that's 1, 16, 256 or 1024 tiles
    const int z = GENERATE(0, 2, 4, 5);

    // level 0 is always loaded too
    std::vector<Tiled2dMapTileInfo> expectedTiles = {{rect, 0, 0, 0, 0, int(zoomLevelInfos[0].zoom)}};
    if (z > 0) {
        assert(zoomLevelInfos[z].zoomLevelIdentifier == z);
        for (int x = 0; x < zoomLevelInfos[z].numTilesX; x++) {
            for (int y = 0; y < zoomLevelInfos[z].numTilesY; y++) {
                expectedTiles.push_back({rect, x, y, 0, z, int(zoomLevelInfos[z].zoom)});
            }
        }
    }

    // This test runs multiple times, with different number of local tiles.
    size_t numExpectedTiles = expectedTiles.size();
    size_t numLocalTiles = GENERATE_COPY(size_t(0), numExpectedTiles, take(4, random(size_t(1), numExpectedTiles)));
    std::vector<Tiled2dMapTileInfo> localTiles;
    std::vector<Tiled2dMapTileInfo> remoteTiles;
    {
        std::vector<Tiled2dMapTileInfo> shuffledTiles = expectedTiles;
        std::mt19937 g{Catch::getSeed()};
        std::shuffle(shuffledTiles.begin(), shuffledTiles.end(), g);
        localTiles.insert(localTiles.end(), shuffledTiles.begin(), shuffledTiles.begin() + numLocalTiles);
        remoteTiles.insert(remoteTiles.end(), shuffledTiles.begin() + numLocalTiles, shuffledTiles.end());
    }

    std::unordered_map<std::string, std::string> localData = generateDummyData(localTiles, *layerConfig, "local data ");
    std::unordered_map<std::string, std::string> remoteData = generateDummyData(remoteTiles, *layerConfig, "remote data ");
    auto localLoader = std::make_shared<BlockingTestLoader>(localData);
    auto remoteLoader = std::make_shared<BlockingTestLoader>(remoteData, 0.25f); // 25% error rate

    auto scheduler = std::make_shared<TestScheduler>();
    std::shared_ptr<TestTiled2dMapVectorSource> source = std::make_shared<TestTiled2dMapVectorSource>(
        layerConfig, scheduler, std::vector<std::shared_ptr<LoaderInterface>>{localLoader, remoteLoader});
    source->mailbox = std::make_shared<Mailbox>(scheduler);

    source->onVisibleBoundsChanged(rect, 0, zoomLevelInfos[z].zoom);

    // Complete all "local" loads
    scheduler->drain();
    REQUIRE(source->numLoadingOrQueued() == expectedTiles.size());
    while (localLoader->unblockAll()) {
        scheduler->drain();
    }

    // These checks will fail if any local loads are blocked by concurrent remote loads.
    auto loadedTileData = source->getCurrentTileUrlsAndData();
    REQUIRE(loadedTileData.size() == localTiles.size());
    REQUIRE_THAT(loadedTileData, Catch::Matchers::UnorderedRangeEquals(localData));
    REQUIRE(source->numLoadingOrQueued() == expectedTiles.size() - localTiles.size());

    // Minutes later ;) Load all the rest...
    scheduler->drain();
    while (localLoader->unblockAll() || remoteLoader->unblockAll()) {
        scheduler->drain();
    }

    std::unordered_map<std::string, std::string> allData{remoteData};
    allData.insert(localData.begin(), localData.end());

    loadedTileData = source->getCurrentTileUrlsAndData();
    REQUIRE(loadedTileData.size() == expectedTiles.size());
    REQUIRE_THAT(loadedTileData, Catch::Matchers::UnorderedRangeEquals(allData));
}

TEST_CASE("Tiled2dMapSource error load retry") {
    
    auto layerConfig = std::make_shared<WebMercatorTiled2dMapLayerConfig>(
        "mock", "test-data://tile/{z}/{x}/{y}", Tiled2dMapZoomInfo(1.0, 0, 0, false, true, false, true), 0, 20);

    // Load the entire world at zoom level 3
    auto rect = *layerConfig->getBounds();
    auto zoomLevelInfos = layerConfig->getZoomLevelInfos();
    const int z = 3;
    std::vector<Tiled2dMapTileInfo> expectedTiles = {{rect, 0, 0, 0, 0, int(zoomLevelInfos[0].zoom)}};
    for (int x = 0; x < zoomLevelInfos[z].numTilesX; x++) {
        for (int y = 0; y < zoomLevelInfos[z].numTilesY; y++) {
            expectedTiles.push_back({rect, x, y, 0, z, int(zoomLevelInfos[z].zoom)});
        }
    }
    REQUIRE(expectedTiles.size() == 64 + 1); // sanity check
   
    auto loader = std::make_shared<BlockingTestLoader>(generateDummyData(expectedTiles, *layerConfig, "dummy data "));

    auto scheduler = std::make_shared<TestScheduler>();
    std::shared_ptr<TestTiled2dMapVectorSource> source =
        std::make_shared<TestTiled2dMapVectorSource>(layerConfig, scheduler, std::vector<std::shared_ptr<LoaderInterface>>{loader});
    source->mailbox = std::make_shared<Mailbox>(scheduler);

    source->onVisibleBoundsChanged(rect, 0, zoomLevelInfos[z].zoom);

    // Let all requests fail
    loader->setErrorRate(1.f);
    while (scheduler->drainUntil(0), loader->unblockAll()) {
    }
    REQUIRE(source->getCurrentTiles().size() == 0);
    REQUIRE(source->numLoadingOrQueued() == 0);

    int64_t time = 0;
    int retry = 0;
    for(; retry <= 10; retry++) {
      int64_t expectedDelay = source->waitTimeAfterRetry(retry);
      
      auto errorTiles = source->getErrorTiles().at(0);
      CHECK(errorTiles.size() == expectedTiles.size());
      for(auto &[tile, errorInfo] : errorTiles) {
          CHECK(errorInfo.delay == expectedDelay);
      }
      time += expectedDelay;
      while (scheduler->drainUntil(time), loader->unblockAll()) {
      }
      REQUIRE(source->numLoadingOrQueued() == 0);
    }
    auto prevTileErrorInfo = source->getErrorTiles().at(0);

    // Now change the camera viewport so we should get a different tile pyramid.
    RectCoord west = rect;
    west.bottomRight.x = 0.f; // only keep left half of world.
    std::vector<Tiled2dMapTileInfo> expectedTilesWest;
    for(auto tile : expectedTiles) {
        if(tile.x <= zoomLevelInfos[z].numTilesX/2) {
            expectedTilesWest.push_back(tile);
        }
    }
    source->onVisibleBoundsChanged(west, 0, zoomLevelInfos[z].zoom);

    // Check that the tiles have _kept_ their error state accross the tile pyramid change:
    REQUIRE(source->numLoadingOrQueued() == 0); // Tiles are not yet loading (instead delayed retry)
    {
      auto errorTiles = source->getErrorTiles().at(0);
      CHECK(errorTiles.size() == expectedTilesWest.size());
      for(auto &[tile, errorInfo] : errorTiles) {
          auto prevErrorInfo = prevTileErrorInfo[tile]; 
          CHECK(errorInfo.delay == prevErrorInfo.delay);
          CHECK(errorInfo.lastLoad == prevErrorInfo.lastLoad);
      }
    }

    // Once loader recovers, loads succeed
    loader->setErrorRate(0.f);
    while (scheduler->drain(), loader->unblockAll()) {
    }

    REQUIRE_THAT(source->getCurrentTiles(), Catch::Matchers::UnorderedRangeEquals(expectedTilesWest));
    REQUIRE(source->numLoadingOrQueued() == 0);
}
