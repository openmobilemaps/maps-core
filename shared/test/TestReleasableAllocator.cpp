#include "ReleasableAllocator.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TestReleasableAllocator<float>") {
    size_t size;
    std::unique_ptr<float> releasedData;

    // Create vec in scope to check that data survives after it is dropped
    {
        std::vector<float, ReleasableAllocator<float>> vec;
        vec.push_back(0.0);
        vec.reserve(8);
        vec.push_back(1.0);

        auto *dataPtr = vec.data();

        size = vec.size();
        releasedData = ReleasableAllocator<float>::release(std::move(vec));
        CHECK(vec.data() == nullptr); // sanity check for std::vector
        REQUIRE(releasedData.get() == dataPtr);
    }

    REQUIRE(size == 2);
    CHECK(releasedData.get()[0] == 0.0);
    CHECK(releasedData.get()[1] == 1.0);
}

TEST_CASE("TestReleasableAllocator<aggregate>") {
    struct TestAggregate {
        float x;
        bool even;
    };

    size_t size;
    std::unique_ptr<TestAggregate> releasedData;

    // Create vec in scope to check that releasedData survives after it is dropped
    {
        std::vector<TestAggregate, ReleasableAllocator<TestAggregate>> vec;
        for(int i = 0; i < 13; i++) {
            vec.push_back({ i * 7.0f, (i%2 == 0)});
        }
        
        auto* dataPtr = vec.data();

        size = vec.size();
        releasedData = ReleasableAllocator<TestAggregate>::release(std::move(vec));
        CHECK(vec.data() == nullptr); // sanity check for std::vector
        REQUIRE(releasedData.get() == dataPtr);
    }

    REQUIRE(size == 13);
    for(int i = 0; i < 13; i++) {
        CHECK(releasedData.get()[i].x == i*7.0f);
        CHECK(releasedData.get()[i].even == (i%2 == 0));
    }
}
