#include "VectorSet.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <set>

TEST_CASE("VectorSet<std::string>::insertSet") {
    VectorSet<std::string> vsA{ "foo", "bar" };
    VectorSet<std::string> vsB{ "bar", "fnord", "food" };
    VectorSet<std::string> vsC{ "fnord" };
    VectorSet<std::string> vsD{ };
    VectorSet<std::string> vsE{ "napf" };
    vsA.insertSet(vsB);
    vsA.insertSet(vsC);
    vsA.insertSet(vsD);
    vsA.insertSet(vsE);

    std::set<std::string> setA(vsA.begin(), vsA.end());
    setA.insert(vsB.begin(), vsB.end());
    setA.insert(vsC.begin(), vsC.end());
    setA.insert(vsD.begin(), vsD.end());
    setA.insert(vsE.begin(), vsE.end());
    REQUIRE_THAT(vsA, Catch::Matchers::UnorderedRangeEquals(setA));
}
