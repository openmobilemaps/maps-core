#include "CoordinateSystemFactory.h"
#include "VectorTileGeometryHandler.h"
#include "CoordinateConversionHelper.h"
#include "helper/TestData.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

struct ParsingResult {
    size_t polygonCount;
    size_t pointCount;
    size_t lineCount;
};

void parseAndTriangulate(const char *filePath, ParsingResult expectedResult, Catch::Benchmark::Chronometer meter) {
    ::RectCoord tileCoords = {Coord(3857,1224991.657211,6287508.342789,0), Coord(3857,1849991.657211,5662508.342789,0)};
    const std::optional<Tiled2dMapVectorSettings> vectorSettings = std::nullopt;
    const auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getEpsg3857System(), false);;

    auto data = TestData::readFileToBuffer(filePath);

    ParsingResult result = {0, 0, 0};
    StringInterner stringTable = ValueKeys::newStringInterner();

    meter.measure([&](int run) {
        vtzero::vector_tile tileData(data.data(), data.size());
        mapbox::detail::Earcut<uint16_t> earcutter;
        while (auto layer = tileData.next_layer()) {
            std::string sourceLayerName = std::string(layer.name());

            while (const auto &feature = layer.next_feature()) {
                int extent = (int) layer.extent();
                auto const featureContext = std::make_shared<FeatureContext>(stringTable, feature);
                VectorTileGeometryHandler geometryHandler = VectorTileGeometryHandler(tileCoords, extent, std::nullopt, conversionHelper);
                decode_geometry(feature.geometry(), geometryHandler);
                size_t polygonCount = geometryHandler.beginTriangulatePolygons();
                for (size_t i = 0; i < polygonCount; i++) {
                    geometryHandler.triangulatePolygons(i, earcutter);
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
        return parseAndTriangulate("tiles/reg.pbf", ParsingResult{318,3336,3336}, meter);
    };
    BENCHMARK_ADVANCED("Benchmark relief")(Catch::Benchmark::Chronometer meter) {
        return parseAndTriangulate("tiles/relief.pbf", ParsingResult{24345,34413,34413}, meter);
    };
}
