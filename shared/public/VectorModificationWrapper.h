/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */
#pragma once

#include <vector>

/**
 * @class VectorWrapper
 * @brief A wrapper for a `std::vector` that tracks modifications.
 *
 * This class provides functionality to wrap a `std::vector` and track
 * whether the vector has been modified. The `modified` flag is set
 * under the following conditions:
 * - Accessing elements via the `operator[]` for writing.
 * - Adding elements using the `push_back()` or `emplace_back()` methods.
 * - resizing the vector via the `resize()` method

 * Use the `resetModificationFlag()` method to reset the modification
 * state after changes are processed.
 *
 * @tparam T The type of elements stored in the vector.
 */
template <typename T>
class VectorModificationWrapper {
  private:
    std::vector<T> vec;
    bool modified;

  public:
    VectorModificationWrapper()
        : modified(false) {}

    explicit VectorModificationWrapper(std::vector<T> initialVec)
        : vec(std::move(initialVec))
        , modified(false) {}

    T &operator[](size_t index) {
        modified = true;
        return vec[index];
    }

    const T &operator[](size_t index) const { return vec[index]; }

    size_t size() const { return vec.size(); }

    T *data() { return vec.data(); }

    const T *data() const { return vec.data(); }

    void resize(size_t newSize) {
        modified = true;
        vec.resize(newSize);
    }

    void resize(size_t newSize, const T &value) {
        modified = true;
        vec.resize(newSize, value);
    }

    bool wasModified() const { return modified; }

    void resetModificationFlag() { modified = false; }

    void setModified() { modified = true; }

    void push_back(const T &value) {
        modified = true;
        vec.push_back(value);
    }

    void push_back(T &&value) {
        modified = true;
        vec.push_back(std::move(value));
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        modified = true;
        vec.emplace_back(std::forward<Args>(args)...);
    }
};
