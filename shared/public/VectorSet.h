#include <algorithm>
#include <vector>

#pragma once

template<typename T>
class VectorSet {
public:
    using const_iterator = typename std::vector<T>::const_iterator;
    using iterator = typename std::vector<T>::iterator;

    VectorSet() = default;
    // Move construction / assignment
    VectorSet(VectorSet<T> &&x) = default;
    VectorSet<T>& operator=(VectorSet<T> &&x) = default;
    // No copy constructor/assignment (BUT see copy())
    VectorSet(const VectorSet<T> &x) = delete;
    VectorSet<T>& operator=(const VectorSet<T> &x) = delete;

    // Constructor with initializer list
    VectorSet(std::initializer_list<T> initList) {
        for (const T& value : initList) {
            insert(value);
        }
    }

    // Explicit copy.
    // This type _is_ copyable, but only with an explicit copy() call to avoid unnecessary, accidental copies.
    VectorSet<T> copy() const {
        VectorSet<T> other;
        other.data = std::vector<T>(data);
        return other;
    }

    // Insert an element into the set
    void insert(const T& value) {
        if (!contains(value)) {
            data.push_back(value);
        }
    }

    // Find an element in the set
    bool contains(const T& value) const {
        return find(value) != data.end();
    }

    const_iterator find(const T& value) const {
        return std::find(data.begin(), data.end(), value);
    }

    // Check if the set is empty
    bool empty() const {
        return data.empty();
    }

    // Check if all elements of another set are present in this set
    bool covers(const VectorSet<T>& other) const {
        for (auto it = other.begin(); it != other.end(); it++) {
            if (!contains(*it)) {
                return false;
            }
        }
        return true;
    }

    // Insert elements from another VectorSet into this set
    void insertSet(const VectorSet<T>& otherSet) {
        size_t oldSize = data.size();
        for (auto it = otherSet.begin(); it != otherSet.end(); it++) {
            // only check if initial set contains a value; 
            // no need to check inserted values also if otherSet is assumed to be duplicate free
            auto searchEnd = data.end() + oldSize;
            if(std::find(data.begin(), searchEnd, *it) != searchEnd) {
                data.push_back(*it);
            }
        }
    }

    // Iterator functions
    const_iterator begin() const {
        return data.begin();
    }

    const_iterator end() const {
        return data.end();
    }

    iterator begin() {
        return data.begin();
    }

    iterator end() {
        return data.end();
    }

    size_t size() const {
        return data.size();
    }

    void reserve(const size_t size) {
        data.reserve(size);
    }

private:
    std::vector<T> data;
};
