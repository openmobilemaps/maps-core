#include "InternedString.h"
#include "StringInterner.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StringInterner base functionality") {
    StringInterner stringTable{};

    std::string strFoo1{"foo"};
    std::string strFoo2 = std::string("f") + "oo";
    auto foo1 = stringTable.add(strFoo1);
    auto foo2 = stringTable.add(strFoo2);
    REQUIRE(foo1 == foo2);
    REQUIRE(foo2 == foo1);
    auto fnord = stringTable.add("fnord");
    REQUIRE(foo1 != fnord);
    REQUIRE(fnord != foo1);

    const std::string &strFooRef1 = stringTable.get(foo1);
    const std::string &strFooRef2 = stringTable.get(foo2);
    REQUIRE(strFooRef1 == "foo");        // Get expected string back
    REQUIRE(&strFooRef1 == &strFooRef2); // Get pointer to same string object every time
    REQUIRE(&strFooRef1 != &strFoo1);    // Its not one of the string objects we initially had
    REQUIRE(&strFooRef1 != &strFoo2);
}

TEST_CASE("StringInterner copy and move") {
    StringInterner baseTable{};
    auto fooBase = baseTable.add("foo");
    auto barBase = baseTable.add("bar");

    StringInterner childTable{baseTable};
    auto fnord = childTable.add("fnord");
    auto barChild = childTable.add("bar");

    REQUIRE(fnord != fooBase);
    REQUIRE(fnord != barBase);
    REQUIRE(barChild == barBase);
    REQUIRE(childTable.get(fooBase) == "foo");

    StringInterner movedTable{std::move(baseTable)};
    REQUIRE(movedTable.get(fooBase) == "foo");
    auto fooMove = movedTable.add("foo");
    REQUIRE(fooBase == fooMove);
}
