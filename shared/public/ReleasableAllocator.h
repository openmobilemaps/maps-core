#pragma once

#include <vector>
#include <memory>
#include <cassert>

/** 
 * ReleasableAllocator is a pseudo-allocator for std::vector, allowing to move
 * the owned data out of a std::vector instance.
 *
 * This implementation only works for trivial types.
 *
 * Usage Example:
 *
 * std::vector<int, ReleasableAllocator<int>> exampleVec;
 * // ... use vector as usual, e.g. exampleVec.push_back(...);
 * std::unique_ptr<int> exampleData = ReleasableAllocator<int>::release(std::move(exampleVec));
 *
 * */
template<class T>
class ReleasableAllocator
{
public:
    using value_type = T;
 
    ReleasableAllocator() 
      : released(nullptr)
    {}

private:
    ReleasableAllocator(T *released) 
      : released(released)
    {}
    
public:
 
    [[nodiscard]] T* allocate(std::size_t n)
    {
        return std::allocator<T>().allocate(n);
    }
    
    void deallocate(T* p, std::size_t n) noexcept
    {
        assert(released == nullptr || p == released);
        if(p == released) {
            return;
        }
        std::allocator<T>().deallocate(p, n);
    }
    
    static std::unique_ptr<T> release(std::vector<T, ReleasableAllocator<T>>&& v) noexcept
    {
        // Note: there is no write-acccess to the allocator instance. 
        // Instead, we create a new vector instance with a modified allocator instance.
        ReleasableAllocator<T> alloc(v.data());
        std::vector<T, ReleasableAllocator<T>> vv(std::move(v), alloc);
        assert(v.data() == nullptr);
        assert(vv.get_allocator().released == alloc.released);
        assert(vv.get_allocator().released == vv.data());
        return std::unique_ptr<T>{vv.data()};
    }

private:
    // Data that should be ignored in a #deallocate call during #release.
    T* released;
};

template<typename T>
bool operator==(const ReleasableAllocator<T>& a, const ReleasableAllocator<T>& b) {
    return true;
}
