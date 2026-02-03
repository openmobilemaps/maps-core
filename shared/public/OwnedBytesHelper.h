#pragma once

#include "OwnedBytes.h"
#include "ReleasableAllocator.h"
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

class OwnedBytesHelper {
public:
    template<typename T>
    static std::pair<std::unique_ptr<T>, size_t> toUniquePtr(const OwnedBytes & bytes) {
        assert(bytes.bytesPerElement == sizeof(T));
        if(bytes.bytesPerElement != sizeof(T)) {
            std::free((void*)bytes.address);
            return std::make_pair(nullptr, 0);
        }
        return std::make_pair(std::unique_ptr<T>(reinterpret_cast<T*>(bytes.address)), bytes.elementCount);
    }

    template<typename T>
    [[nodiscard]] 
    static OwnedBytes fromVector(std::vector<T, ReleasableAllocator<T>> &&vec) {
        size_t size = vec.size();
        auto ptr = ReleasableAllocator<T>::release(std::move(vec));
        return fromUniquePtr(std::move(ptr), size);
    }
    
    template<typename T>
    [[nodiscard]] 
    static OwnedBytes fromUniquePtr(std::unique_ptr<T> ptr, size_t size) {
        const int32_t elementCount = static_cast<int32_t>(size);
        const int32_t bytesPerElement = sizeof(T);
        return OwnedBytes(
            reinterpret_cast<int64_t>(ptr.release()), 
            elementCount, 
            bytesPerElement);
    }
};
