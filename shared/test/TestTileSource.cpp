#include "CoordinateConversionHelper.h"
#include "CoordinateSystemFactory.h"
#include "DataLoaderResult.h"
#include "Epsg3857Tiled2dMapLayerConfig.h"
#include "LoaderInterface.h"
#include "MapCamera3dMode.h"
#include "Matrix.h"
#include "MatrixD.h"
#include "TextureLoaderResult.h"
#include "Tiled2dMap3dDistanceBasedTerrainSelector.h"
#include "Tiled2dMapSource.h"
#include "helper/TestScheduler.h"

#include "Tiled2dMapSourceImpl.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <deque>
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

    // Constructor for the 3D camera path (onCameraChange), which needs a real render-system aware
    // conversion helper and a globe map coordinate system.
    TestTiled2dMapVectorSource(MapConfig mapConfig, std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper,
                               std::shared_ptr<Tiled2dMapLayerConfig> layerConfig, std::shared_ptr<TestScheduler> scheduler,
                               std::vector<std::shared_ptr<LoaderInterface>> loaders)
        : Tiled2dMapSource(mapConfig, layerConfig, conversionHelper, scheduler, 62, loaders.size(), "layer")
        , layerConfig(layerConfig)
        , loaders(loaders) {}

    // Last tile pyramid emitted by the selector, captured for assertions.
    std::vector<VisibleTilesLayer> capturedLayers;

    size_t capturedTileCount() const {
        size_t n = 0;
        for (const auto &layer : capturedLayers) {
            n += layer.visibleTiles.size();
        }
        return n;
    }

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

    int64_t waitTimeAfterRetry(int retry) const { return std::min((1 << retry) * MIN_WAIT_TIME, MAX_WAIT_TIME); }

    void onVisibleTilesChangedForTest(const std::vector<VisibleTilesLayer> &pyramid, bool enforceMultipleLevels,
                                      int keepZoomLevelOffset = 0) {
        onVisibleTilesChanged(pyramid, enforceMultipleLevels, keepZoomLevelOffset);
    }

    std::unordered_map<size_t, std::map<Tiled2dMapTileInfo, ErrorInfo>> getErrorTiles() const { return errorTiles; }

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
        , errorRate(errorRate) {}

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
        auto &load = blockedLoads.emplace_back(BlockedLoad{url, djinni::Promise<DataLoaderResult>()});
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

    void setErrorRate(float errorRate_) { errorRate = errorRate_; }

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

static std::shared_ptr<Epsg3857Tiled2dMapLayerConfig> createTestLayerConfig(int maxZoomLevel = 20) {
    auto zoomInfo = Tiled2dMapVectorLayerConfig::defaultMapZoomInfo();
    zoomInfo.adaptScaleToScreen =
        false; // Important, otherwise the onVisibleBoundsChanged does not pick up the (in the tests) expected zoom level.
    return std::make_shared<Epsg3857Tiled2dMapLayerConfig>("mock", "test-data://tile/{z}/{x}/{y}", std::nullopt, zoomInfo,
                                                           Tiled2dMapVectorLayerConfig::generateLevelsFromMinMax(0, maxZoomLevel));
}

TEST_CASE("VectorTileSource") {
    auto layerConfig = createTestLayerConfig();
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

static std::vector<Tiled2dMapTileInfo> generateSyntheticTiles(size_t tileCount) {
    std::vector<Tiled2dMapTileInfo> tiles;
    tiles.reserve(tileCount);

    const int zoomIdentifier = 10;
    const int zoomLevel = 10;
    const int side = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(tileCount))));
    const double tileSize = 1.0;

    for (size_t i = 0; i < tileCount; i++) {
        const int x = static_cast<int>(i % side);
        const int y = static_cast<int>(i / side);
        const Coord topLeft(3857, x * tileSize, y * tileSize, 0);
        const Coord bottomRight(3857, (x + 1) * tileSize, (y + 1) * tileSize, 0);
        Tiled2dMapTileInfo tile(RectCoord(topLeft, bottomRight), x, y, 0, zoomIdentifier, zoomLevel);
        tiles.push_back(tile);
    }

    return tiles;
}

static std::vector<VisibleTilesLayer> generateSyntheticTilePyramid(size_t tileCount) {
    VisibleTilesLayer layer(0, 0);
    auto tiles = generateSyntheticTiles(tileCount);

    for (size_t i = 0; i < tiles.size(); i++) {
        layer.visibleTiles.insert(PrioritizedTiled2dMapTileInfo(tiles[i], static_cast<int>(i % 1024)));
    }

    return {layer};
}

TEST_CASE("Tiled2dMapSource scheduler benchmark", "[!benchmark]") {
    auto layerConfig = createTestLayerConfig();
    const auto pyramid = generateSyntheticTilePyramid(4096);
    const auto drainTiles = generateSyntheticTiles(16384);

    BENCHMARK_ADVANCED("visible tile queue update, 4096 tiles")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            auto scheduler = std::make_shared<TestScheduler>();
            auto loader = std::make_shared<BlockingTestLoader>(std::unordered_map<std::string, std::string>{});
            auto source = std::make_shared<TestTiled2dMapVectorSource>(layerConfig, scheduler,
                                                                       std::vector<std::shared_ptr<LoaderInterface>>{loader});
            source->mailbox = std::make_shared<Mailbox>(scheduler);
            source->onVisibleTilesChangedForTest(pyramid, false);
            return source->numLoadingOrQueued();
        });
    };

    BENCHMARK_ADVANCED("queue drain old vector erase-front, 16384 tiles")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            auto queue = drainTiles;
            size_t drained = 0;
            while (!queue.empty()) {
                drained += static_cast<size_t>(queue.front().x);
                queue.erase(queue.begin());
            }
            return drained;
        });
    };

    BENCHMARK_ADVANCED("queue drain deque pop-front, 16384 tiles")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&] {
            std::deque<Tiled2dMapTileInfo> queue(drainTiles.begin(), drainTiles.end());
            size_t drained = 0;
            while (!queue.empty()) {
                drained += static_cast<size_t>(queue.front().x);
                queue.pop_front();
            }
            return drained;
        });
    };
}

/**
 * Test that loading tiles from a fallback "remote" loader does not block a primary "local" loader.
 * "local" loads here are simmulated to return immediately, while "remote"
 * loads are slower than -- the first remote load returns after the last local
 * load.
 * This test is repeated for different number of total tiles (zoom level) and fraction of local tiles, via Catch GENERATE.
 */
TEST_CASE("Tiled2dMapSource slow fallback does not block local loads") {

    auto layerConfig = createTestLayerConfig();

    // Load the entire world.
    auto rect = CoordinateSystemFactory::getEpsg3857System().bounds;
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
    auto remoteLoader = std::make_shared<BlockingTestLoader>(remoteData);

    auto scheduler = std::make_shared<TestScheduler>();
    std::shared_ptr<TestTiled2dMapVectorSource> source = std::make_shared<TestTiled2dMapVectorSource>(
        layerConfig, scheduler, std::vector<std::shared_ptr<LoaderInterface>>{localLoader, remoteLoader});
    source->mailbox = std::make_shared<Mailbox>(scheduler);

    source->onVisibleBoundsChanged(rect, 0, zoomLevelInfos[z].zoom);

    // Complete all "local" loads
    scheduler->drain();
    CAPTURE(z);
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
    auto layerConfig = createTestLayerConfig();

    // Load the entire world at zoom level 3
    auto rect = CoordinateSystemFactory::getEpsg3857System().bounds;
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

    auto prevTileErrorInfo = source->getErrorTiles().at(0);
    CHECK(prevTileErrorInfo.size() == expectedTiles.size());
    for (auto &[tile, errorInfo] : prevTileErrorInfo) {
        CHECK(errorInfo.delay == source->waitTimeAfterRetry(0));
    }

    // Now change the camera viewport so we should get a different tile pyramid.
    RectCoord west = rect;
    west.bottomRight.x = 0.f; // only keep left half of world.
    std::vector<Tiled2dMapTileInfo> expectedTilesWest;
    for (auto tile : expectedTiles) {
        if (tile.x <= zoomLevelInfos[z].numTilesX / 2) {
            expectedTilesWest.push_back(tile);
        }
    }
    source->onVisibleBoundsChanged(west, 0, zoomLevelInfos[z].zoom);

    // Check that the tiles have kept their error state across the tile pyramid change:
    REQUIRE(source->numLoadingOrQueued() == 0); // Tiles are not yet loading (instead delayed retry)
    {
        auto errorTiles = source->getErrorTiles().at(0);
        CHECK(errorTiles.size() == expectedTilesWest.size());
        for (auto &[tile, errorInfo] : errorTiles) {
            auto prevErrorInfo = prevTileErrorInfo[tile];
            CHECK(errorInfo.delay == prevErrorInfo.delay);
            CHECK(errorInfo.lastLoad == prevErrorInfo.lastLoad);
        }
    }

    // Once loader recovers, loads succeed
    loader->setErrorRate(0.f);
    source->forceReload();
    while (scheduler->drain(), loader->unblockAll()) {
    }

    REQUIRE_THAT(source->getCurrentTiles(), Catch::Matchers::UnorderedRangeEquals(expectedTilesWest));
    REQUIRE(source->numLoadingOrQueued() == 0);
}

namespace {

struct TestCameraMatrices {
    std::vector<float> viewMatrix;
    std::vector<float> projectionMatrix;
    Vec3D origin{0, 0, 0};
    Vec3D cameraPosition{0, 0, 0};
    float verticalFov = 0;
    float horizontalFov = 0;
};

// Builds a top-down orbit camera on the unit-sphere globe, replicating the matrix construction of
// MapCamera3d::computeMatrices (orbit branch) with zero padding, pitch and rotation.
static TestCameraMatrices makeOrbitCamera(double longitude, double latitude, double cameraDistance, double fovyDeg, double aspect,
                                          double cameraPitch = 0.0, double angle = 0.0) {
    TestCameraMatrices cam;
    cam.viewMatrix.assign(16, 0.0f);
    cam.projectionMatrix.assign(16, 0.0f);

    Matrix::perspectiveM(cam.projectionMatrix, 0, (float)fovyDeg, (float)aspect, (float)(cameraDistance - 1.0),
                         (float)(cameraDistance + 1.0));

    const double lo = (longitude - 180.0) * M_PI / 180.0;
    const double la = (latitude - 90.0) * M_PI / 180.0;
    const double x = std::sin(la) * std::cos(lo);
    const double y = std::cos(la);
    const double z = -(std::sin(la) * std::sin(lo));

    Matrix::setIdentityM(cam.viewMatrix, 0);
    Matrix::translateM(cam.viewMatrix, 0, 0.0f, 0.0f, (float)-cameraDistance);
    Matrix::rotateM(cam.viewMatrix, 0, (float)-cameraPitch, 1.0f, 0.0f, 0.0f);
    Matrix::rotateM(cam.viewMatrix, 0, (float)-angle, 0.0f, 0.0f, 1.0f);
    Matrix::translateM(cam.viewMatrix, 0, 0.0f, 0.0f, -1.0f); // -1 - focusPointAltitude/R (alt 0)
    Matrix::rotateM(cam.viewMatrix, 0, (float)latitude, 1.0f, 0.0f, 0.0f);
    Matrix::rotateM(cam.viewMatrix, 0, (float)-longitude, 0.0f, 1.0f, 0.0f);
    Matrix::rotateM(cam.viewMatrix, 0, -90.0f, 0.0f, 1.0f, 0.0f);
    Matrix::translateM(cam.viewMatrix, 0, (float)x, (float)y, (float)z);

    cam.origin = Vec3D(x, y, z);

    std::vector<float> inverseView(16, 0.0f);
    Matrix::invertM(inverseView, 0, cam.viewMatrix, 0);
    Vec4D cameraH = Matrix::multiply(inverseView, Vec4D(0.0, 0.0, 0.0, 1.0));
    cam.cameraPosition = Vec3D(cameraH.x / cameraH.w, cameraH.y / cameraH.w, cameraH.z / cameraH.w);

    cam.verticalFov = (float)fovyDeg;
    cam.horizontalFov = (float)(fovyDeg * aspect);
    return cam;
}

// Captures the selected tile pyramid without forwarding to the real load scheduling, so the selection
// logic (onCameraChange) can be tested in isolation without the full actor/loader setup.
class GlobeTestSource : public TestTiled2dMapVectorSource {
  public:
    using TestTiled2dMapVectorSource::TestTiled2dMapVectorSource;

    void onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid, bool keepMultipleLevels,
                               int keepZoomLevelOffset = 0) override {
        capturedLayers = pyramid;
    }
};

class TerrainGlobeTestSource : public GlobeTestSource {
  public:
    using GlobeTestSource::GlobeTestSource;

  protected:
    using TestSourceBase = Tiled2dMapSource<std::shared_ptr<DataLoaderResult>, std::string>;

    const Tiled2dMap3dTileDetailSelector &get3dTileDetailSelector() const override { return terrainDetailSelector; }

    const Tiled2dMap3dTileSelection<TestSourceBase> &get3dTileSelection() const override { return *tileSelection; }

    Tiled2dMap3dDistanceBasedTerrainSelector terrainDetailSelector;
    std::unique_ptr<Tiled2dMap3dTileSelection<TestSourceBase>> tileSelection =
        makeDisplacedTerrainTiled2dMap3dTileSelection<TestSourceBase>();
};

static std::shared_ptr<GlobeTestSource> makeGlobeSource(std::shared_ptr<TestScheduler> scheduler,
                                                        std::vector<std::shared_ptr<LoaderInterface>> loaders) {
    auto mapConfig = MapConfig(CoordinateSystemFactory::getUnitSphereSystem());
    auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getUnitSphereSystem(), false);
    // A shallow level range keeps the refinement walk bounded for the synthetic test cameras (which pair a
    // fixed zoom with varying distances); the selection logic under test is the same at any depth.
    return std::make_shared<GlobeTestSource>(mapConfig, conversionHelper, createTestLayerConfig(/*maxZoomLevel*/ 6), scheduler,
                                             loaders);
}

static std::shared_ptr<TerrainGlobeTestSource> makeTerrainGlobeSource(std::shared_ptr<TestScheduler> scheduler,
                                                                      std::vector<std::shared_ptr<LoaderInterface>> loaders) {
    auto mapConfig = MapConfig(CoordinateSystemFactory::getUnitSphereSystem());
    auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getUnitSphereSystem(), false);
    return std::make_shared<TerrainGlobeTestSource>(mapConfig, conversionHelper, createTestLayerConfig(/*maxZoomLevel*/ 6),
                                                    scheduler, loaders);
}

} // namespace

TEST_CASE("Tiled2dMapSource orbit camera selects tiles") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};
    auto source = makeGlobeSource(scheduler, loaders);

    const auto cam = makeOrbitCamera(/*longitude*/ 8.0, /*latitude*/ 47.0, /*cameraDistance*/ 1.2, /*fovyDeg*/ 42.0,
                                     /*aspect*/ 1.0);

    source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov,
                           /*width*/ 1000.0f, /*height*/ 1000.0f, /*focusPointAltitude*/ 0.0f,
                           Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.0, 47.0, 0.0), /*zoom*/ 100000.0, cam.cameraPosition,
                           MapCamera3dMode::ORBIT);

    // The orbit camera looking at the globe must select a non-empty set of terrain tiles.
    CHECK(source->capturedTileCount() > 0);
}

TEST_CASE("Tiled2dMapSource orbit camera skips recompute for unchanged input") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};
    auto source = makeGlobeSource(scheduler, loaders);

    const auto cam = makeOrbitCamera(8.0, 47.0, 1.2, 42.0, 1.0);
    const auto focus = Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.0, 47.0, 0.0);

    source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f, 1000.0f,
                           0.0f, focus, 100000.0, cam.cameraPosition, MapCamera3dMode::ORBIT);
    const size_t firstCount = source->capturedTileCount();
    REQUIRE(firstCount > 0);

    // Same inputs again: the selection is unchanged, so the walk is skipped but the previously captured
    // selection must remain valid (non-empty).
    source->capturedLayers.clear();
    source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f, 1000.0f,
                           0.0f, focus, 100000.0, cam.cameraPosition, MapCamera3dMode::ORBIT);
    CHECK(source->capturedLayers.empty()); // early-out: no re-emit

    // A moved camera recomputes and selects tiles again.
    const auto cam2 = makeOrbitCamera(9.5, 46.0, 1.2, 42.0, 1.0);
    source->onCameraChange(cam2.viewMatrix, cam2.projectionMatrix, cam2.origin, cam2.verticalFov, cam2.horizontalFov, 1000.0f,
                           1000.0f, 0.0f, Coord(CoordinateSystemIdentifiers::EPSG4326(), 9.5, 46.0, 0.0), 100000.0,
                           cam2.cameraPosition, MapCamera3dMode::ORBIT);
    CHECK(source->capturedTileCount() > 0);
}

TEST_CASE("Tiled2dMapSource orbit camera refines both sides of the antimeridian") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};

    auto selectTiles = [&](double longitude) {
        auto source = makeTerrainGlobeSource(scheduler, loaders);
        const auto cam = makeOrbitCamera(longitude, /*latitude*/ 0.0, /*cameraDistance*/ 1.05, /*fovyDeg*/ 42.0,
                                         /*aspect*/ 1.0);
        source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f,
                               1000.0f, 0.0f, Coord(CoordinateSystemIdentifiers::EPSG4326(), longitude, 0.0, 0.0), 100000.0,
                               cam.cameraPosition, MapCamera3dMode::ORBIT);
        return source;
    };

    auto maxSelectedLevel = [](const std::shared_ptr<GlobeTestSource> &source) {
        int maxLevel = -1;
        for (const auto &layer : source->capturedLayers) {
            for (const auto &tile : layer.visibleTiles) {
                maxLevel = std::max(maxLevel, tile.tileInfo.zoomIdentifier);
            }
        }
        return maxLevel;
    };

    // The same equatorial view away from the seam defines the expected detail level.
    const auto reference = selectTiles(0.0);
    const int referenceMaxLevel = maxSelectedLevel(reference);
    REQUIRE(referenceMaxLevel > 0);

    // Just east of the antimeridian, the ground across the seam is equally close and must not lose detail.
    const auto seam = selectTiles(179.5);
    const int seamMaxLevel = maxSelectedLevel(seam);
    CHECK(seamMaxLevel == referenceMaxLevel);

    const int numTilesX = 1 << seamMaxLevel;
    bool eastOfSeamAtMaxLevel = false; // last column, longitude just below +180
    bool westOfSeamAtMaxLevel = false; // first column, longitude just above -180
    for (const auto &layer : seam->capturedLayers) {
        for (const auto &tile : layer.visibleTiles) {
            if (tile.tileInfo.zoomIdentifier != seamMaxLevel) {
                continue;
            }
            eastOfSeamAtMaxLevel = eastOfSeamAtMaxLevel || tile.tileInfo.x == numTilesX - 1;
            westOfSeamAtMaxLevel = westOfSeamAtMaxLevel || tile.tileInfo.x == 0;
        }
    }
    CHECK(eastOfSeamAtMaxLevel);
    CHECK(westOfSeamAtMaxLevel);
}

TEST_CASE("Tiled2dMapSource orbit camera near the pole selects coarser levels than at the equator") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};

    auto maxSelectedLevelAt = [&](double latitude) {
        auto source = makeTerrainGlobeSource(scheduler, loaders);
        const auto cam = makeOrbitCamera(/*longitude*/ 8.0, latitude, /*cameraDistance*/ 1.05, /*fovyDeg*/ 42.0,
                                         /*aspect*/ 1.0);
        source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f,
                               1000.0f, 0.0f, Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.0, latitude, 0.0), 100000.0,
                               cam.cameraPosition, MapCamera3dMode::ORBIT);
        int maxLevel = -1;
        for (const auto &layer : source->capturedLayers) {
            for (const auto &tile : layer.visibleTiles) {
                maxLevel = std::max(maxLevel, tile.tileInfo.zoomIdentifier);
            }
        }
        return maxLevel;
    };

    const int equatorMaxLevel = maxSelectedLevelAt(0.0);
    REQUIRE(equatorMaxLevel > 0);

    // At latitude 80 a mercator tile covers cos(80°) ~ 1/6 of the ground it covers at the equator, so the
    // same ground resolution is reached ~2.5 levels earlier; without that correction polar tiles oversample.
    const int polarMaxLevel = maxSelectedLevelAt(80.0);
    CHECK(polarMaxLevel < equatorMaxLevel);
    CHECK(polarMaxLevel <= equatorMaxLevel - 2);
}

TEST_CASE("Tiled2dMapSource tilted orbit camera selects tiles") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};

    // A tilted, closer orbit camera (as used for the 3D terrain view) must still select tiles.
    for (double pitch : {20.0, 40.0, 60.0}) {
        for (double distance : {1.05, 1.2, 2.0}) {
            const auto cam = makeOrbitCamera(8.0, 47.0, distance, 42.0, 1.0, pitch, 30.0);
            auto source = makeTerrainGlobeSource(scheduler, loaders);
            source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f,
                                   1000.0f, 0.0f, Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.0, 47.0, 0.0), 100000.0,
                                   cam.cameraPosition, MapCamera3dMode::ORBIT);
            CAPTURE(pitch, distance);
            CHECK(source->capturedTileCount() > 0);
        }
    }
}

namespace {

// A pyramid whose levels are not integer multiples of each other (2x2 -> 3x3 -> 5x5), as allowed
// by custom WMTS configurations.
class NonDoublingPyramidLayerConfig : public Tiled2dMapLayerConfig {
  public:
    int32_t getCoordinateSystemIdentifier() override { return CoordinateSystemIdentifiers::EPSG3857(); }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        return "test-data://tile/" + std::to_string(zoom) + "/" + std::to_string(x) + "/" + std::to_string(y);
    }

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override {
        const auto bounds = CoordinateSystemFactory::getEpsg3857System().bounds;
        const float worldWidth = std::abs(bounds.bottomRight.x - bounds.topLeft.x);
        return {
            Tiled2dMapZoomLevelInfo(500000000.0, worldWidth / 2.0f, 2, 2, 1, 0, bounds),
            Tiled2dMapZoomLevelInfo(250000000.0, worldWidth / 3.0f, 3, 3, 1, 1, bounds),
            Tiled2dMapZoomLevelInfo(125000000.0, worldWidth / 5.0f, 5, 5, 1, 2, bounds),
        };
    }

    std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override { return {}; }

    Tiled2dMapZoomInfo getZoomInfo() override {
        auto zoomInfo = Tiled2dMapVectorLayerConfig::defaultMapZoomInfo();
        zoomInfo.adaptScaleToScreen = false;
        return zoomInfo;
    }

    std::string getLayerName() override { return "mock"; }

    std::optional<Tiled2dMapVectorSettings> getVectorSettings() override { return std::nullopt; }

    std::optional<::RectCoord> getBounds() override { return std::nullopt; }
};

} // namespace

TEST_CASE("Tiled2dMapSource refines non-doubling pyramids completely") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};
    auto mapConfig = MapConfig(CoordinateSystemFactory::getUnitSphereSystem());
    auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getUnitSphereSystem(), false);
    auto source = std::make_shared<TerrainGlobeTestSource>(mapConfig, conversionHelper,
                                                           std::make_shared<NonDoublingPyramidLayerConfig>(), scheduler, loaders);

    const auto cam = makeOrbitCamera(/*longitude*/ 8.0, /*latitude*/ 47.0, /*cameraDistance*/ 1.2, /*fovyDeg*/ 42.0,
                                     /*aspect*/ 1.0);
    source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f, 1000.0f,
                           0.0f, Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.0, 47.0, 0.0), 100000.0, cam.cameraPosition,
                           MapCamera3dMode::ORBIT);

    REQUIRE(source->capturedTileCount() > 0);

    // The last row/column of the 3x3 level is only reachable when children are derived geometrically:
    // integer tile-count ratios (3/2 == 1) would never generate x == 2 or y == 2.
    bool foundLastRowOrColumn = false;
    for (const auto &layer : source->capturedLayers) {
        std::unordered_set<Tiled2dMapTileInfo> uniqueTiles;
        for (const auto &tile : layer.visibleTiles) {
            const auto &info = tile.tileInfo;
            if (info.zoomIdentifier == 1) {
                CHECK(info.x >= 0);
                CHECK(info.x < 3);
                CHECK(info.y >= 0);
                CHECK(info.y < 3);
                foundLastRowOrColumn |= info.x == 2 || info.y == 2;
            }
            // Every tile must be selected exactly once per layer.
            CHECK(uniqueTiles.insert(info).second);
        }
    }
    CHECK(foundLastRowOrColumn);
}

TEST_CASE("Tiled2dMapSource tile loading pause is independent of lifecycle pause") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};
    auto source = makeGlobeSource(scheduler, loaders);

    const auto cam = makeOrbitCamera(8.0, 47.0, 1.2, 42.0, 1.0);
    const auto focus = Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.0, 47.0, 0.0);
    source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, 1000.0f, 1000.0f,
                           0.0f, focus, 100000.0, cam.cameraPosition, MapCamera3dMode::ORBIT);
    REQUIRE(source->capturedTileCount() > 0);

    source->setTileLoadingPaused(true);
    source->capturedLayers.clear();

    const auto cam2 = makeOrbitCamera(9.5, 46.0, 1.2, 42.0, 1.0);
    const auto focus2 = Coord(CoordinateSystemIdentifiers::EPSG4326(), 9.5, 46.0, 0.0);
    source->onCameraChange(cam2.viewMatrix, cam2.projectionMatrix, cam2.origin, cam2.verticalFov, cam2.horizontalFov, 1000.0f,
                           1000.0f, 0.0f, focus2, 100000.0, cam2.cameraPosition, MapCamera3dMode::ORBIT);
    CHECK(source->capturedLayers.empty());

    // A lifecycle resume (e.g. app foregrounding) must not cancel the debug pause.
    source->resume();
    source->onCameraChange(cam2.viewMatrix, cam2.projectionMatrix, cam2.origin, cam2.verticalFov, cam2.horizontalFov, 1000.0f,
                           1000.0f, 0.0f, focus2, 100000.0, cam2.cameraPosition, MapCamera3dMode::ORBIT);
    CHECK(source->capturedLayers.empty());

    // Conversely, ending the debug pause while lifecycle-paused must not resume tile selection.
    source->pause();
    source->setTileLoadingPaused(false);
    source->onCameraChange(cam2.viewMatrix, cam2.projectionMatrix, cam2.origin, cam2.verticalFov, cam2.horizontalFov, 1000.0f,
                           1000.0f, 0.0f, focus2, 100000.0, cam2.cameraPosition, MapCamera3dMode::ORBIT);
    CHECK(source->capturedLayers.empty());

    source->resume();
    source->onCameraChange(cam2.viewMatrix, cam2.projectionMatrix, cam2.origin, cam2.verticalFov, cam2.horizontalFov, 1000.0f,
                           1000.0f, 0.0f, focus2, 100000.0, cam2.cameraPosition, MapCamera3dMode::ORBIT);
    CHECK(source->capturedTileCount() > 0);
}

namespace {

struct PoseTestCamera {
    std::vector<float> viewMatrix;
    std::vector<float> projectionMatrix;
    Vec3D origin{0, 0, 0};
    Vec3D cameraPosition{0, 0, 0}; // origin-relative, as onCameraChange expects
    Vec3D cameraWorld{0, 0, 0};    // absolute unit-sphere position
    Vec3D forward{0, 0, 0};
    Vec3D up{0, 0, 0};
    Vec3D right{0, 0, 0};
    float verticalFov = 0;
    float horizontalFov = 0;
    Coord focus = Coord(CoordinateSystemIdentifiers::EPSG4326(), 0, 0, 0);
    double zoom = 0;
};

static Coord unitSphereCartesianToWgs84SurfaceForTest(const Vec3D &cartesian) {
    const auto up = Vec3DHelper::normalize(cartesian);
    double longitude = std::atan2(up.x, up.z) * 180.0 / M_PI - 90.0;
    if (longitude < -180.0)
        longitude += 360.0;
    if (longitude > 180.0)
        longitude -= 360.0;
    const double latitude = std::asin(std::clamp(up.y, -1.0, 1.0)) * 180.0 / M_PI;
    return Coord(CoordinateSystemIdentifiers::EPSG4326(), longitude, latitude, 0.0);
}

// Builds a first-person (pose) camera standing at the given position/altitude, heading north, pitched
// down from the horizontal by pitchDownDeg. Mirrors the pose branch of MapCamera3d::computeMatrices.
static PoseTestCamera makePoseCamera(double lonDeg, double latDeg, double altitudeMeters, double pitchDownDeg, double fovyDeg,
                                     double aspect, double farPlaneMeters, double screenDensityPpi, double widthPx) {
    constexpr double R = 6378137.0;
    PoseTestCamera cam;

    const double lo = (lonDeg - 180.0) * M_PI / 180.0;
    const double la = (latDeg - 90.0) * M_PI / 180.0;
    const Vec3D n0(std::sin(la) * std::cos(lo), std::cos(la), -(std::sin(la) * std::sin(lo)));

    const Vec3D poleAxis(0.0, 1.0, 0.0);
    const Vec3D north = Vec3DHelper::normalize(poleAxis - n0 * (poleAxis.x * n0.x + poleAxis.y * n0.y + poleAxis.z * n0.z));

    const double p = pitchDownDeg * M_PI / 180.0;
    cam.forward = Vec3DHelper::normalize(north * std::cos(p) - n0 * std::sin(p));
    cam.up = Vec3DHelper::normalize(n0 * std::cos(p) + north * std::sin(p));
    cam.right = Vec3DHelper::normalize(Vec3DHelper::crossProduct(cam.forward, cam.up));

    const double h = altitudeMeters / R;
    cam.cameraWorld = n0 * (1.0 + h);
    cam.origin = n0;
    const Vec3D eye = cam.cameraWorld - cam.origin;
    cam.cameraPosition = eye;

    std::vector<double> view(16, 0.0);
    const Vec3D center = eye + cam.forward;
    MatrixD::setLookAtM(view, 0, eye.x, eye.y, eye.z, center.x, center.y, center.z, cam.up.x, cam.up.y, cam.up.z);

    std::vector<double> proj(16, 0.0);
    const double nearPlane = std::max(1.0, altitudeMeters * 0.1) / R;
    MatrixD::perspectiveM(proj, 0, fovyDeg, aspect, nearPlane, farPlaneMeters / R);

    cam.viewMatrix.assign(view.begin(), view.end());
    cam.projectionMatrix.assign(proj.begin(), proj.end());
    cam.verticalFov = (float)fovyDeg;
    cam.horizontalFov = (float)(2.0 * std::atan(std::tan(fovyDeg * M_PI / 360.0) * aspect) * 180.0 / M_PI);

    // Screen-center ground hit for focus/zoom, like getPoseSurfacePosition/getPoseDerivedZoom. A center
    // ray in the sky falls back to the camera footprint.
    const double cd = cam.cameraWorld.x * cam.forward.x + cam.cameraWorld.y * cam.forward.y + cam.cameraWorld.z * cam.forward.z;
    const double cc = Vec3DHelper::length(cam.cameraWorld) * Vec3DHelper::length(cam.cameraWorld);
    const double disc = cd * cd - (cc - 1.0);
    Vec3D focusWorld = n0;
    double centerDistance = altitudeMeters;
    if (disc >= 0.0) {
        const double t = -cd - std::sqrt(disc);
        if (t > 0.0) {
            focusWorld = cam.cameraWorld + cam.forward * t;
            centerDistance = t * R;
        }
    }
    cam.focus = unitSphereCartesianToWgs84SurfaceForTest(focusWorld);

    const double pixelsPerMeter = screenDensityPpi / 0.0254;
    const double horizontalFovRadians = cam.horizontalFov * M_PI / 180.0;
    cam.zoom = 2.0 * std::tan(horizontalFovRadians / 2.0) * std::max(centerDistance, 1.0) * pixelsPerMeter / widthPx;
    return cam;
}

} // namespace

TEST_CASE("Tiled2dMapSource flat pose view covers all visible ground") {
    auto scheduler = std::make_shared<TestScheduler>();
    std::vector<std::shared_ptr<LoaderInterface>> loaders{std::make_shared<NothingTestLoader>()};
    auto helper = CoordinateConversionHelperInterface::independentInstance();

    const double width = 1000.0, height = 1000.0;
    const double altitude = GENERATE(1500.0, 8000.0);
    const double pitchDown = GENERATE(0.5, 3.0, 30.0);
    CAPTURE(altitude, pitchDown);

    auto mapConfig = MapConfig(CoordinateSystemFactory::getUnitSphereSystem());
    auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getUnitSphereSystem(), false);
    auto source = std::make_shared<GlobeTestSource>(mapConfig, conversionHelper, createTestLayerConfig(10), scheduler, loaders);

    const auto cam = makePoseCamera(8.0, 47.0, altitude, pitchDown, 60.0, width / height, 4000000.0, 62.0, width);

    source->onCameraChange(cam.viewMatrix, cam.projectionMatrix, cam.origin, cam.verticalFov, cam.horizontalFov, (float)width,
                           (float)height, (float)cam.focus.z, cam.focus, (float)cam.zoom, cam.cameraPosition,
                           MapCamera3dMode::POSE);
    REQUIRE(source->capturedTileCount() > 0);

    // Collect the EPSG3857 bounds of every selected tile (all pyramid layers).
    std::vector<std::array<double, 4>> tileRects;
    for (const auto &layer : source->capturedLayers) {
        for (const auto &tile : layer.visibleTiles) {
            const auto &b = tile.tileInfo.bounds;
            tileRects.push_back({std::min(b.topLeft.x, b.bottomRight.x), std::max(b.topLeft.x, b.bottomRight.x),
                                 std::min(b.topLeft.y, b.bottomRight.y), std::max(b.topLeft.y, b.bottomRight.y)});
        }
    }

    // Cast rays through a screen grid; every visible ground hit must be inside some selected tile.
    const double tanHalfV = std::tan(cam.verticalFov * M_PI / 360.0);
    const double tanHalfH = std::tan(cam.horizontalFov * M_PI / 360.0);
    int groundRays = 0;
    int uncovered = 0;
    std::string firstUncovered;
    for (int iy = 0; iy <= 20; ++iy) {
        for (int ix = 0; ix <= 20; ++ix) {
            const double ndcX = ix / 10.0 - 1.0;
            const double ndcY = iy / 10.0 - 1.0;
            const Vec3D dir = Vec3DHelper::normalize(cam.forward + cam.right * (ndcX * tanHalfH) + cam.up * (ndcY * tanHalfV));
            const double cd = cam.cameraWorld.x * dir.x + cam.cameraWorld.y * dir.y + cam.cameraWorld.z * dir.z;
            const double cc = cam.cameraWorld.x * cam.cameraWorld.x + cam.cameraWorld.y * cam.cameraWorld.y +
                              cam.cameraWorld.z * cam.cameraWorld.z;
            const double disc = cd * cd - (cc - 1.0);
            if (disc < 0.0) {
                continue; // sky
            }
            const double t = -cd - std::sqrt(disc);
            if (t <= 0.0) {
                continue;
            }
            const Vec3D hit = cam.cameraWorld + dir * t;
            const auto hit4326 = unitSphereCartesianToWgs84SurfaceForTest(hit);
            const auto hit3857 = helper->convert(CoordinateSystemIdentifiers::EPSG3857(), hit4326);
            groundRays++;

            bool covered = false;
            for (const auto &rect : tileRects) {
                if (hit3857.x >= rect[0] && hit3857.x <= rect[1] && hit3857.y >= rect[2] && hit3857.y <= rect[3]) {
                    covered = true;
                    break;
                }
            }
            if (!covered) {
                uncovered++;
                if (firstUncovered.empty()) {
                    const double distMeters = t * 6378137.0;
                    firstUncovered = "ndc(" + std::to_string(ndcX) + "," + std::to_string(ndcY) + ") dist " +
                                     std::to_string(distMeters / 1000.0) + "km";
                }
            }
        }
    }
    CAPTURE(groundRays, uncovered, firstUncovered);
    REQUIRE(groundRays > 0);
    CHECK(uncovered == 0);
}
