#include "CoordinateConversionHelper.h"
#include "CoordinateSystemFactory.h"
#include "ValueKeys.h"
#include "VectorTileDataParser.h"

#include "helper/TestData.h"

#include <catch2/catch_test_macros.hpp>

namespace {

RectCoord getTileBounds() {
    return {Coord(3857, 1224991.657211, 6287508.342789, 0), Coord(3857, 1849991.657211, 5662508.342789, 0)};
}

Tiled2dMapTileInfo getTileInfo() {
    return Tiled2dMapTileInfo(getTileBounds(), 33, 22, 0, 6, 6);
}

size_t featureCount(const Tiled2dMapVectorTileInfo::FeatureMap &featureMap) {
    size_t total = 0;
    for (const auto &[layerName, features] : *featureMap) {
        total += features->size();
    }
    return total;
}

} // namespace

TEST_CASE("VectorTileDataParser parses MVT tiles", "[VectorTileDataParser]") {
    const auto tileData = TestData::readFileToBuffer("tiles/reg.pbf");
    auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getEpsg3857System(), false);
    StringInterner stringTable = ValueKeys::newStringInterner();

    auto featureMap = VectorTileDataParser::parse(reinterpret_cast<const uint8_t *>(tileData.data()),
                                                  tileData.size(),
                                                  getTileInfo(),
                                                  getTileBounds(),
                                                  std::nullopt,
                                                  conversionHelper,
                                                  stringTable,
                                                  {},
                                                  VectorTileSourceFormat::MVT,
                                                  [] { return true; });

    REQUIRE(featureMap);
    REQUIRE(!featureMap->empty());
    REQUIRE(featureCount(featureMap) > 0);
}

TEST_CASE("VectorTileDataParser parses MLT tiles", "[VectorTileDataParser]") {
    const auto tileData = TestData::readFileToBuffer("tiles/polygon_multi.mlt");
    auto conversionHelper = std::make_shared<CoordinateConversionHelper>(CoordinateSystemFactory::getEpsg3857System(), false);
    StringInterner stringTable = ValueKeys::newStringInterner();

    auto featureMap = VectorTileDataParser::parse(reinterpret_cast<const uint8_t *>(tileData.data()),
                                                  tileData.size(),
                                                  getTileInfo(),
                                                  getTileBounds(),
                                                  std::nullopt,
                                                  conversionHelper,
                                                  stringTable,
                                                  {},
                                                  VectorTileSourceFormat::MLT,
                                                  [] { return true; });

    REQUIRE(featureMap);
    REQUIRE(!featureMap->empty());
    REQUIRE(featureCount(featureMap) > 0);
}

