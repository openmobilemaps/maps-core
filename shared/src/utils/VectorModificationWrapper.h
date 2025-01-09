/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <vector>

template <typename T>
class VectorModificationWrapper {
private:
    std::vector<T> vec;
    bool modified;

public:
    VectorModificationWrapper() : modified(false) {}

    explicit VectorModificationWrapper(std::vector<T> initialVec)
        : vec(std::move(initialVec)), modified(false) {}

    T& operator[](size_t index) {
        modified = true;
        return vec[index];
    }

    const T& operator[](size_t index) const {
        return vec[index];
    }

    size_t size() const {
        return vec.size();
    }

    T* data() {
        return vec.data();
    }

    const T* data() const {
        return vec.data();
    }

    void resize(size_t newSize) {
        modified = true;
        vec.resize(newSize);
    }

    bool wasModified() const {
        return modified;
    }

    void resetModificationFlag() {
        modified = false;
    }

    void push_back(const T& value) {
        modified = true;
        vec.push_back(value);
    }

    void push_back(T&& value) {
        modified = true;
        vec.push_back(std::move(value));
    }
};
