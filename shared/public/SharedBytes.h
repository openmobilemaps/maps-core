// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#pragma once

#include <cstdint>
#include <utility>

struct SharedBytes final {
    int64_t address;
    int32_t elementCount;
    int32_t bytesPerElement;

    SharedBytes(int64_t address_,
                int32_t elementCount_,
                int32_t bytesPerElement_)
    : address(std::move(address_))
    , elementCount(std::move(elementCount_))
    , bytesPerElement(std::move(bytesPerElement_))
    {}
};
