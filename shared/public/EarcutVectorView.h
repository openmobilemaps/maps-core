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

#include "Coord.h"
#include "vtzero/geometry.hpp"

struct PolygonRing {
    std::vector<vtzero::point> points;
    bool isHole = false;
};

struct PolygonRange {
    size_t startIndex;
    size_t size;

    size_t numberOfHoles() { return size - 1; }
};

template <typename T>
class SignedArea {
protected:
    static double signedArea(const std::vector<T>& hole) {
        double sum = 0;
        for (std::size_t i = 0, len = hole.size(), j = len - 1; i < len; j = i++) {
            const auto& p1 = hole[i];
            const auto& p2 = hole[j];
            sum += (p2.x - p1.x) * (p1.y + p2.y);
        }
        return sum;
    }
};

class EarcutCoordVectorView : public SignedArea<Coord> {
public:
    EarcutCoordVectorView(std::vector<Coord>& polygonPoints,
                          std::vector<std::vector<Coord>>& holes,
                          std::size_t maxHoles)
        : polygonPoints(polygonPoints), originalHoles(holes)
    {
        if (originalHoles.size() > maxHoles) {
            copiedHoles = originalHoles; // copy all
            std::nth_element(
                copiedHoles.begin(), copiedHoles.begin() + maxHoles, copiedHoles.end(),
                [](const std::vector<Coord>& a, const std::vector<Coord>& b) {
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

    const std::vector<Coord>& operator[](std::size_t index) const {
        if (index == 0) {
            return polygonPoints;
        } else {
            return useCopy ? copiedHoles[index - 1] : originalHoles[index - 1];
        }
    }

private:
    const std::vector<Coord>& polygonPoints;
    const std::vector<std::vector<Coord>>& originalHoles;
    std::vector<std::vector<Coord>> copiedHoles;
    bool useCopy = false;
};


class EarcutPolygonVectorView : public SignedArea<vtzero::point> {
public:
    EarcutPolygonVectorView(std::vector<PolygonRing>& polygonRings,
                            PolygonRange& range, std::size_t maxHoles)
    : polygonRings(polygonRings), range(range) {
        if(range.numberOfHoles() > maxHoles) {
            copiedHoles.reserve(range.size - 1);
            for(auto i = range.startIndex + 1; i < range.startIndex + range.size; ++i) {
                copiedHoles.push_back(polygonRings[i]);
            }

            std::nth_element(
                copiedHoles.begin(), copiedHoles.begin() + maxHoles, copiedHoles.end(),
                [](const PolygonRing& a, const PolygonRing& b) {
                    return std::fabs(signedArea(a.points)) > std::fabs(signedArea(b.points));
                });
            copiedHoles.resize(maxHoles);
            useCopy = true;
        }
    }

    bool empty() const {
        return size() == 0;
    }

    std::size_t size() const {
        return useCopy ? (copiedHoles.size() + 1) : range.size;
    }

    const std::vector<vtzero::point>& operator[](std::size_t index) const {
        if (index == 0) {
            return polygonRings[range.startIndex].points;
        } else {
            return useCopy ? copiedHoles[index - 1].points : polygonRings[range.startIndex + index].points;
        }
    }

private:
    const std::vector<PolygonRing>& polygonRings;
    const PolygonRange& range;

    std::vector<PolygonRing> copiedHoles;
    bool useCopy = false;
};
