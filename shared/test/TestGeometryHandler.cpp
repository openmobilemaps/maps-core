#include "CoordinateSystemFactory.h"
#include "DefaultSystemToRenderConverter.h"
#include "SymbolAnimationCoordinatorMap.h"
#include "VectorTileGeometryHandler.h"

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

std::vector<char> readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Unable to open file");
    }

    std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    if (!file.read(buffer.data(), fileSize)) {
        throw std::runtime_error("Error reading file");
    }

    return buffer;
}

struct ParsingResult {
    size_t polygonCount;
    size_t pointCount;
    size_t lineCount;
};

void parseAndTriangulate(const std::string& filePath, ParsingResult expectedResult, Catch::Benchmark::Chronometer meter) {
    ::RectCoord tileCoords = {Coord(3857,1224991.657211,6287508.342789,0), Coord(3857,1849991.657211,5662508.342789,0)};
    const std::optional<Tiled2dMapVectorSettings> vectorSettings = std::nullopt;
    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper = CoordinateConversionHelperInterface::independentInstance();

    conversionHelper->registerConverter(std::make_shared<DefaultSystemToRenderConverter>(CoordinateSystemFactory::getEpsg3857System(), false));

    auto data= readFile(filePath);

    ParsingResult result = {0, 0, 0};

    meter.measure([&](int run) {
        vtzero::vector_tile tileData(data.data(), data.size());
        while (auto layer = tileData.next_layer()) {
            std::string sourceLayerName = std::string(layer.name());

            while (const auto &feature = layer.next_feature()) {
                int extent = (int) layer.extent();
                auto const featureContext = std::make_shared<FeatureContext>(feature);
                VectorTileGeometryHandler geometryHandler = VectorTileGeometryHandler(tileCoords, extent, std::nullopt, conversionHelper);
                decode_geometry(feature.geometry(), geometryHandler);
                size_t polygonCount = geometryHandler.beginTriangulatePolygons();
                for (size_t i = 0; i < polygonCount; i++) {
                    geometryHandler.triangulatePolygons(i);
                }
                geometryHandler.endTringulatePolygons();
                result.polygonCount += geometryHandler.getPolygons().size();
                result.pointCount += geometryHandler.getPointCoordinates().size();
                result.lineCount += geometryHandler.getLineCoordinates().size();
            }
        }
    });

    CHECK(result.polygonCount == meter.runs() * expectedResult.polygonCount);
    CHECK(result.pointCount == meter.runs() * expectedResult.pointCount);
    CHECK(result.lineCount == meter.runs() * expectedResult.lineCount);
}

TEST_CASE("VectorTileGeometryHandler") {
    BENCHMARK_ADVANCED("Benchmark regular")(Catch::Benchmark::Chronometer meter) {
        return parseAndTriangulate("data/tiles/reg.pbf", ParsingResult{318,3336,3336}, meter);
    };
    BENCHMARK_ADVANCED("Benchmark relief")(Catch::Benchmark::Chronometer meter) {
        parseAndTriangulate("data/tiles/relief.pbf", ParsingResult{24345,34413,34413}, meter);
    };
}
