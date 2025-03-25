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

template<typename T>
class EarcutVectorView {
public:
    EarcutVectorView(std::vector<T>& polygonPoints,
                     std::vector<std::vector<T>>& holes,
                     std::size_t maxHoles)
        : polygonPoints(polygonPoints), originalHoles(holes)
    {
        if (originalHoles.size() > maxHoles) {
            copiedHoles = originalHoles; // copy all
            std::nth_element(
                copiedHoles.begin(), copiedHoles.begin() + maxHoles, copiedHoles.end(),
                [](const std::vector<T>& a, const std::vector<T>& b) {
                    return std::fabs(signedArea(a)) > std::fabs(signedArea(b));
                });
            copiedHoles.resize(maxHoles);
            useCopy = true;
        }
    }

    bool empty() const {
        return size() == 0;
    }

    std::size_t size() const {
        return 1 + (useCopy ? copiedHoles.size() : originalHoles.size());
    }

    const std::vector<T>& operator[](std::size_t index) const {
        if (index == 0) {
            return polygonPoints;
        } else {
            return useCopy ? copiedHoles[index - 1] : originalHoles[index - 1];
        }
    }

private:
    static double signedArea(const std::vector<T>& hole) {
        double sum = 0;
        for (std::size_t i = 0, len = hole.size(), j = len - 1; i < len; j = i++) {
            const auto& p1 = hole[i];
            const auto& p2 = hole[j];
            sum += (p2.x - p1.x) * (p1.y + p2.y);
        }
        return sum;
    }

private:
    std::vector<T>& polygonPoints;
    std::vector<std::vector<T>>& originalHoles;
    std::vector<std::vector<T>> copiedHoles;
    bool useCopy = false;
};
