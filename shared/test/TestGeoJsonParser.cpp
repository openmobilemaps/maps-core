#include "GeoJsonParser.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fstream>
#include <random>

static std::string readFileToString(const char *path) {
    std::ifstream ifs{path};
    std::string content{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    if (ifs.fail()) {
        throw std::runtime_error(std::string("Error reading file ") + path + ": " + strerror(errno));
    }
    return content;
}

/** Helper to capture std::ostream (std::cout or std::cerr) into a string.
 * Captures writes to stream until destroyed.
 */
class OstreamCapture {
  public:
    OstreamCapture(std::ostream &stream_)
        : stream(stream_)
        , origBuf(stream.rdbuf()) {
        stream.rdbuf(capture.rdbuf());
    }

    ~OstreamCapture() { stream.rdbuf(origBuf); }

    std::string get() const { return capture.str(); }

  private:
    std::ostream &stream;
    std::stringstream capture;
    std::streambuf *origBuf;
};

TEST_CASE("GeoJSON Parser valid") {
    struct ExpectedGeometry {
        vtzero::GeomType type;
        std::vector<size_t> numCoordinates;
        std::vector<size_t> numHoles;
        size_t numProperties;
    };
    struct TestCase {
        const char *file;
        std::vector<ExpectedGeometry> expectedGeometries;
        bool expectedHasOnlyPoints;
        size_t expectedNumErrorMessages;
    };

    auto testCase = GENERATE(
        TestCase{
            .file = "data/geojson/featurecollection.json",
            .expectedGeometries =
                {
                    {
                        .type = vtzero::GeomType::POINT,
                        .numCoordinates = {1},
                        .numProperties = 1,
                    },
                    {
                        .type = vtzero::GeomType::LINESTRING,
                        .numCoordinates = {4},
                        .numProperties = 2,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {5},
                        .numHoles = {0},
                        .numProperties = 2,
                    },
                },
            .expectedHasOnlyPoints = false,
        },
        TestCase{
            .file = "data/geojson/featurecollection_noprops.json",
            .expectedGeometries =
                {
                    {
                        .type = vtzero::GeomType::POINT,
                        .numCoordinates = {1},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::LINESTRING,
                        .numCoordinates = {4},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {5},
                        .numHoles = {0},
                        .numProperties = 0,
                    },
                },
            .expectedHasOnlyPoints = false,
        },
        TestCase{
            .file = "data/geojson/featurecollection_partially-invalid.json",
            .expectedGeometries =
                {
                    // 0: invalid Feature/Point is skipped
                    // 1: invalid Point is skipped
                    // 2: Point is accepted, even if "type": "Feature" is missing
                    {
                        .type = vtzero::GeomType::POINT,
                        .numCoordinates = {1},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::LINESTRING,
                        .numCoordinates = {4},
                        .numProperties = 2,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {5},
                        .numHoles = {0},
                        .numProperties = 2,
                    },
                },
            .expectedHasOnlyPoints = false,
            .expectedNumErrorMessages = 2,
        },
        TestCase{
            .file = "data/geojson/featurecollection_all-types.json",
            .expectedGeometries =
                {
                    {
                        .type = vtzero::GeomType::POINT,
                        .numCoordinates = {1},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::LINESTRING,
                        .numCoordinates = {3},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {5},
                        .numHoles = {0},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {5},
                        .numHoles = {1},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::POINT,
                        .numCoordinates =
                            {
                                1,
                                1,
                                1,
                                1,
                            },
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::LINESTRING,
                        .numCoordinates = {3, 4},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {4, 5},
                        .numHoles = {0, 0},
                        .numProperties = 0,
                    },
                    {
                        .type = vtzero::GeomType::POLYGON,
                        .numCoordinates = {4, 6},
                        .numHoles = {0, 2},
                        .numProperties = 0,
                    },
                    // [8] skipped, unsupported GeometryCollection
                },
            .expectedHasOnlyPoints = false,
            .expectedNumErrorMessages = 1,
        },
        TestCase{
            .file = "data/geojson/feature.json",
            .expectedGeometries = {{
                .type = vtzero::GeomType::POINT,
                .numCoordinates = {1},
                .numProperties = 6, // 8 defined in json, 2 skipped (empty array, object property)
            }},
            .expectedHasOnlyPoints = true,
        },
        TestCase{
            .file = "data/geojson/point.json",
            .expectedGeometries = {{
                .type = vtzero::GeomType::POINT,
                .numCoordinates = {1},
                .numProperties = 0,
            }},
            .expectedHasOnlyPoints = true,
        },
        TestCase{
            .file = "data/geojson/multipolygon.json",
            .expectedGeometries = {{
                .type = vtzero::GeomType::POLYGON,
                .numCoordinates = {4, 6},
                .numHoles = {0, 2},
                .numProperties = 0,
            }},
            .expectedHasOnlyPoints = false,
        });

    SECTION(testCase.file) {
        const std::string jsonString = readFileToString(testCase.file);
        const auto json = nlohmann::json::parse(jsonString);

        std::string log;
        std::shared_ptr<GeoJson> geoJson;
        {
            OstreamCapture logCapture(std::cout);
            geoJson = GeoJsonParser::getGeoJson(json);
            log = logCapture.get();
        }

        REQUIRE(geoJson->geometries.size() == testCase.expectedGeometries.size());
        for (size_t i = 0; i < testCase.expectedGeometries.size(); i++) {
            const auto &actual = geoJson->geometries[i];
            const auto &expected = testCase.expectedGeometries[i];
            CHECK(actual->featureContext->geomType == expected.type);
            // Check number of coordinates for each nested object:
            std::vector<size_t> actualNumCoordinates;
            for (auto &objCoordinates : actual->coordinates) {
                actualNumCoordinates.push_back(objCoordinates.size());
            }
            CHECK(actualNumCoordinates == expected.numCoordinates);
            if (expected.type == vtzero::GeomType::POLYGON) {
                REQUIRE(actual->holes.size() == actual->coordinates.size());
                // Check number of holes for each nested polygon:
                std::vector<size_t> actualNumHoles;
                for (auto &objHoles : actual->holes) {
                    actualNumHoles.push_back(objHoles.size());
                }
                CHECK(actualNumHoles == expected.numHoles);
            } else {
                CHECK(actual->holes.empty());
            }
            //  num properties: +2, because type and id are added as internal properties
            CHECK(actual->featureContext->propertiesMap.size() == expected.numProperties + 2);
        }
        CHECK(geoJson->hasOnlyPoints == testCase.expectedHasOnlyPoints);

        size_t numErrorMessages = 0;
        const char *errorStr = "ERROR: ";
        for (size_t it = log.find(errorStr); it != std::string::npos; it = log.find(errorStr, it + 1)) {
            ++numErrorMessages;
        }
        CAPTURE(log);
        CHECK(numErrorMessages == testCase.expectedNumErrorMessages);
    }
}

TEST_CASE("GeoJSON Parser invalid") {
    auto file = GENERATE("data/geojson/invalid/featurecollection_features-not-array.json",
                         "data/geojson/invalid/feature_unsupported-mixed-type-array.json",
                         "data/geojson/invalid/geometrycollection_unsupported.json", "data/geojson/invalid/point_coords-short.json",
                         "data/geojson/invalid/point_coords-string.json");

    SECTION(file) {
        const std::string jsonString = readFileToString(file);
        // files are valid json, no exception expected here.
        auto json = nlohmann::json::parse(jsonString);
        REQUIRE_THROWS_AS(GeoJsonParser::getGeoJson(json), nlohmann::json::exception);
    }
}

TEST_CASE("GeoJSON Parser Points") {
    SECTION("skip everything but Point") {
        auto file = "data/geojson/featurecollection_all-types.json";
        const std::string jsonString = readFileToString(file);
        auto json = nlohmann::json::parse(jsonString);
        auto points = GeoJsonParser::getPointsWithProperties(json);
        REQUIRE(points.size() == 1);
        REQUIRE(points[0].featureInfo.identifier == "point0");
    }

    SECTION("properties") {
        auto file = "data/geojson/featurecollection_points.json";
        const std::string jsonString = readFileToString(file);
        auto json = nlohmann::json::parse(jsonString);
        std::string log;
        std::vector<GeoJsonPoint> points;
        {
            OstreamCapture logCapture{std::cout};
            points = GeoJsonParser::getPointsWithProperties(json);
            log = logCapture.get();
        }
        REQUIRE(points.size() == 3);
        CHECK(points[0].featureInfo.identifier == "point0");
        CHECK(points[0].featureInfo.properties.size() == 0);

        CHECK(points[1].featureInfo.identifier == "point1");
        CHECK(points[1].featureInfo.properties.size() == 0);

        CHECK(points[2].featureInfo.identifier == "point2");
        auto &props2 = points[2].featureInfo.properties;
        CHECK(props2.at("prop_string").stringVal == "duh");
        CHECK(props2.at("prop_number_integer").intVal == 1);
        CHECK(props2.at("prop_number_float").doubleVal == 1.5);
        CHECK(props2.at("prop_bool").boolVal == false);
        CHECK(props2.at("prop_string_array").listStringVal == std::vector<std::string>{"this", "that"});
        CHECK(props2.at("prop_number_array").listFloatVal == std::vector<float>{1.f, 2.f, 3.f});
        CHECK(props2.size() == 6);

        CAPTURE(log);
        CHECK(std::count(log.begin(), log.end(), '\n') == 1);
    }

    SECTION("benchmark") {
        // generate a geojson with many points.
        const size_t numPoints = 10000;
        nlohmann::json geoJson;
        {
            std::default_random_engine rndE{Catch::getSeed()};
            std::uniform_real_distribution<float> distr(-10000.f, 10000.f);

            geoJson["type"] = "FeatureCollection";
            for (size_t i = 0; i < numPoints; i++) {
                auto &feature = geoJson["features"][i];
                feature["type"] = "Feature";
                feature["id"] = std::string("point") + std::to_string(i);

                feature["geometry"]["type"] = "Point";
                feature["geometry"]["coordinates"] = std::vector<float>{distr(rndE), distr(rndE)},

                feature["properties"]["testvalue"] = std::string("property_value_") + std::to_string(i);
            }
        }
        // sanity check
        {
            auto points = GeoJsonParser::getPointsWithProperties(geoJson);
            REQUIRE(points.size() == numPoints);
        }

        BENCHMARK("load many points") { return GeoJsonParser::getPointsWithProperties(geoJson); };
    }
}

TEST_CASE("GeoJSON Parser Lines") {
    SECTION("skip everything but LineString") {
        auto file = "data/geojson/featurecollection_all-types.json";
        const std::string jsonString = readFileToString(file);
        auto json = nlohmann::json::parse(jsonString);
        auto lines = GeoJsonParser::getLinesWithProperties(json);
        REQUIRE(lines.size() == 1);
        REQUIRE(lines[0].featureInfo.identifier == "line0");
        REQUIRE(lines[0].points.size() == 3);
    }

    SECTION("properties") {
        auto file = "data/geojson/featurecollection_lines.json";
        const std::string jsonString = readFileToString(file);
        auto json = nlohmann::json::parse(jsonString);
        std::string log;
        std::vector<GeoJsonLine> lines;
        {
            OstreamCapture logCapture{std::cout};
            lines = GeoJsonParser::getLinesWithProperties(json);
            log = logCapture.get();
        }
        REQUIRE(lines.size() == 3);
        CHECK(lines[0].featureInfo.identifier == "line0");
        CHECK(lines[0].points.size() == 3);
        CHECK(lines[0].featureInfo.properties.size() == 0);

        CHECK(lines[1].featureInfo.identifier == "line1");
        CHECK(lines[1].points.size() == 4);
        CHECK(lines[1].featureInfo.properties.size() == 0);

        CHECK(lines[2].featureInfo.identifier == "line2");
        CHECK(lines[2].points.size() == 5);
        auto &props2 = lines[2].featureInfo.properties;
        CHECK(props2.at("prop_string").stringVal == "value0");
        CHECK(props2.at("prop_number_integer").intVal == 2);
        CHECK(props2.at("prop_number_float").doubleVal == 1.5);
        CHECK(props2.at("prop_bool").boolVal == false);
        CHECK(props2.at("prop_string_array").listStringVal == std::vector<std::string>{"this", "that", "and what about this?"});
        CHECK(props2.at("prop_number_array").listFloatVal == std::vector<float>{1.f, 2.f, 3.f});
        CHECK(props2.size() == 6);

        CAPTURE(log);
        CHECK(std::count(log.begin(), log.end(), '\n') == 1);
    }
}
