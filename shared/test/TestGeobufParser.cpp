#include "GeobufParser.h"
#include "helper/TestData.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Geobuf Parser FeatureCollection") {
    auto geobuf = TestData::readFileToBuffer("geobuf/featurecollection_point_line.pbf");
    StringInterner stringTable = ValueKeys::newStringInterner();

    auto geoJson = GeobufParser::getGeoJson(::djinni::DataRef(geobuf.data(), geobuf.size()), stringTable);
    REQUIRE(geoJson != nullptr);
    REQUIRE(geoJson->geometries.size() == 2);
    CHECK(geoJson->hasOnlyPoints == false);

    const auto &point = geoJson->geometries[0];
    CHECK(point->featureContext->geomType == vtzero::GeomType::POINT);
    REQUIRE(point->coordinates.size() == 1);
    REQUIRE(point->coordinates[0].size() == 1);
    CHECK(point->coordinates[0][0].x == Catch::Approx(8.0));
    CHECK(point->coordinates[0][0].y == Catch::Approx(47.0));

    const auto pointInfo = point->featureContext->getFeatureInfo(stringTable);
    CHECK(pointInfo.properties.at("name").stringVal == "point");

    const auto &line = geoJson->geometries[1];
    CHECK(line->featureContext->geomType == vtzero::GeomType::LINESTRING);
    REQUIRE(line->coordinates.size() == 1);
    REQUIRE(line->coordinates[0].size() == 3);
    CHECK(line->coordinates[0][2].x == Catch::Approx(8.2));
    CHECK(line->coordinates[0][2].y == Catch::Approx(47.2));
}

TEST_CASE("Geobuf Parser Geometry") {
    auto geobuf = TestData::readFileToBuffer("geobuf/line_geometry.pbf");
    StringInterner stringTable = ValueKeys::newStringInterner();

    auto geoJson = GeobufParser::getGeoJson(::djinni::DataRef(geobuf.data(), geobuf.size()), stringTable);
    REQUIRE(geoJson != nullptr);
    REQUIRE(geoJson->geometries.size() == 1);
    CHECK(geoJson->hasOnlyPoints == false);

    const auto &line = geoJson->geometries[0];
    CHECK(line->featureContext->geomType == vtzero::GeomType::LINESTRING);
    REQUIRE(line->coordinates.size() == 1);
    REQUIRE(line->coordinates[0].size() == 3);
    CHECK(line->coordinates[0][0].x == Catch::Approx(8.0));
    CHECK(line->coordinates[0][0].y == Catch::Approx(47.0));
    CHECK(line->coordinates[0][2].x == Catch::Approx(8.2));
    CHECK(line->coordinates[0][2].y == Catch::Approx(47.2));
}
