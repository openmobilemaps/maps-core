#include "TestInterface.h"

::TestEnum TestInterface::test(int32_t value) {
    return value % 2 == 0 ? TestEnum::TEST_1 : TestEnum::TEST_2;
}