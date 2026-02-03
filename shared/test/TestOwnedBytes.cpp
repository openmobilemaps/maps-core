#include "OwnedBytesHelper.h"
#include "OwnedBytesDestructor.h"

#include <stdio.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("OwnedBytes to unique ptr") {
    
    OwnedBytes ownedBytes(0, 0, 0);
    {
        std::vector<float, ReleasableAllocator<float>> vec(3);
        vec[0] = 9.0;
        vec[1] = 13.0;
        vec[2] = 19.0;
        float* vecData = vec.data();
        ownedBytes = OwnedBytesHelper::fromVector(std::move(vec));
        CHECK(vec.data() == nullptr);
        REQUIRE(ownedBytes.address == reinterpret_cast<uintptr_t>(vecData));
    }

    {
        auto [data, size] = OwnedBytesHelper::toUniquePtr<float>(ownedBytes);
        REQUIRE(size == 3);
        CHECK(data.get()[0] == 9.0);
        CHECK(data.get()[1] == 13.0);
        CHECK(data.get()[2] == 19.0);
    }
}

TEST_CASE("OwnedBytes to OwnedBytesDestructor") {
    struct FizzBuzz {
        // intentionally stupid order for alignment
        bool fizz;
        float num;
        bool buzz;
    };
    
    OwnedBytes ownedBytes(0, 0, 0);
    {
        std::vector<FizzBuzz, ReleasableAllocator<FizzBuzz>> vec;
        for(int i = 0; i < 99; ++i) {
            vec.push_back(FizzBuzz{
                .fizz = (i % 3) == 0,
                .num = (float) i,
                .buzz = (i % 5) == 0
            });
        }
        FizzBuzz* vecData = vec.data();
        ownedBytes = OwnedBytesHelper::fromVector(std::move(vec));
        CHECK(vec.data() == nullptr);
        REQUIRE(ownedBytes.address == reinterpret_cast<uintptr_t>(vecData));
    }


    {
        const FizzBuzz* data = reinterpret_cast<const FizzBuzz*>(ownedBytes.address);
        REQUIRE(ownedBytes.elementCount == 99);
        for(int i = 0; i < 99; ++i) {
            CHECK(data[i].fizz == ((i % 3) == 0));
            CHECK(data[i].num == (float)i);
            CHECK(data[i].buzz == ((i % 5) == 0));
        }
    }
    OwnedBytesDestructor::free(ownedBytes);
}
