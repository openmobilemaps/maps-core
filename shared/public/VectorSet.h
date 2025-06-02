#include <algorithm>
#include <memory>
#include <vector>

#pragma once

template<typename T>
class VectorSet {
public:
    using const_iterator = typename std::vector<T>::const_iterator;
    using iterator = typename std::vector<T>::iterator;

    // Default constructor
    VectorSet(): data(std::make_shared<std::vector<T>>()) {}

    VectorSet(const VectorSet<T>& source)
         : data(std::move(source.data))
    {  }

    // Constructor with initializer list
    VectorSet(std::initializer_list<T> initList)
    : data(std::make_shared<std::vector<T>>()) {
        for (const T& value : initList) {
            insert(value);
        }
    }

    // Insert an element into the set
    void insert(const T& value) {
        if (!contains(value)) {
            data->push_back(value);
        }
    }

    // Find an element in the set
    bool contains(const T& value) const {
        return find(value) != data->end();
    }

    const_iterator find(const T& value) const {
        return std::find(data->begin(), data->end(), value);
    }

    // Check if the set is empty
    bool empty() const {
        return data->empty();
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
        for (auto it = otherSet.begin(); it != otherSet.end(); it++) {
            insert(*it);
        }
    }

    // Iterator functions
    iterator begin() const {
        return data->begin();
    }

    iterator end() const {
        return data->end();
    }

    size_t size() const {
        return data->size();
    }

    void reserve(const size_t size) {
        data->reserve(size);
    }

private:
    std::shared_ptr<std::vector<T>> data;
};
