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

#include "Matrix.h"
#include "RectD.h"
#include "Vec3D.h"
#include "CircleD.h"
#include "CollisionPrimitives.h"
#include "CollisionUtil.h"
#include <vector>

struct IndexRange {
public:
    int16_t xMin = std::numeric_limits<int16_t>::max();
    int16_t xMax = std::numeric_limits<int16_t>::min();
    int16_t yMin = std::numeric_limits<int16_t>::max();
    int16_t yMax = std::numeric_limits<int16_t>::min();

    void addXIndex(int16_t index, int16_t maxX) {
        if (index < xMin) {
            xMin = std::max(index, int16_t(0));
        }
        if (index > xMax) {
            xMax = std::min(index, maxX);
        }
    }

    void addYIndex(int16_t index, int16_t maxY) {
        if (index < yMin) {
            yMin = std::max(index, int16_t(0));
        }
        if (index > yMax) {
            yMax = std::min(index, maxY);
        }
    }

    bool isValid(int16_t maxX, int16_t maxY) const {
        return xMin <= maxX && xMax >= 0 && yMin <= maxY && yMax >= 0;
    }
};

class CollisionGrid {
public:
    CollisionGrid(const std::vector<double> &vpMatrix, const Vec2I &size, float gridAngle, bool alwaysInsert, bool is3d, const Vec3D &origin)
            : vpMatrix(vpMatrix), size(size),
              sinNegGridAngle(std::sin(-gridAngle * M_PI / 180.0)),
              cosNegGridAngle(std::cos(-gridAngle * M_PI / 180.0)),
              alwaysInsert(alwaysInsert),
              is3d(is3d),
              origin(origin) {
        cellSize = std::min(size.x, size.y) / (float) numCellsMinDim;
        numCellsX = (cellSize > 0 ? std::ceil(size.x / cellSize) : 0.0) + 2 * numCellsPadding;
        numCellsY = (cellSize > 0 ? std::ceil(size.y / cellSize) : 0.0) + 2 * numCellsPadding;
        halfWidth = size.x / 2.0f;
        halfHeight = size.y / 2.0f;
        gridRects.reserve(numCellsY);
        gridCircles.reserve(numCellsY);
        for (size_t i = 0; i < numCellsY; i++) {
            gridRects.emplace_back(numCellsX);
            gridCircles.emplace_back(numCellsX);
        }
    }

    /**
     * Add a collision grid aligned rectangle (when projected with the provided vpMatrix) and receive the feedback,
     * if it has collided with the previous content of the grid. Only added, when not colliding!
     * return true (1) if collision, or true (2) if outside of bounds
     */
    uint8_t addAndCheckCollisionAlignedRect(const CollisionRectD &rectangle) {
        CollisionUtil::CollisionEnvironment env(vpMatrix, is3d, temp1, temp2, halfWidth, halfHeight, sinNegGridAngle, cosNegGridAngle, origin);
        const auto &projectedRectangle = CollisionUtil::getProjectedRectangle(rectangle, env);
        if (!projectedRectangle) {
            return 2;
        }
        const IndexRange &indexRange = getIndexRangeForRectangle(*projectedRectangle);
        if (!indexRange.isValid(numCellsX - 1, numCellsY - 1)) {
            return 2; // Fully outside of bounds - not relevant
        }

        if (rectangle.contentHash != 0 && rectangle.symbolSpacing > 0) {
            const auto &equalRects = spacedRects.find(rectangle.contentHash);
            if (equalRects != spacedRects.end()) {
                for (const auto &other : equalRects->second) {
                    // Assume equal symbol spacing for all primitives with matching content
                    if (CollisionUtil::checkRectCollision(*projectedRectangle, other, rectangle.symbolSpacing)) {
                        return 1;
                    }
                }
            }
            const auto &equalCircles = spacedCircles.find(rectangle.contentHash);
            if (equalCircles != spacedCircles.end()) {
                for (const auto &other : equalCircles->second) {
                    // Assume equal symbol spacing for all primitives with matching content
                    if (CollisionUtil::checkRectCircleCollision(*projectedRectangle, other, rectangle.symbolSpacing)) {
                        return 1;
                    }
                }
            }
        }

        if (alwaysInsert) {
            return checkRectInsertAlways(rectangle, *projectedRectangle, indexRange);
        } else {
            return checkRectInsertOnCollision(rectangle, *projectedRectangle, indexRange);
        }
    }

    /**
    * Add a vector of circles (which are then projected with the provided vpMatrix) and receive the feedback, if they have collided
    * with the previous content of the grid. Assumed to remain circles in the projected space. Only added, when not colliding!
    */
    uint8_t addAndCheckCollisionCircles(const std::vector<CollisionCircleD> &circles) {
        if (circles.empty()) {
            // No circles -> no colliding
            return 0;
        }

        std::vector<std::tuple<CircleD, IndexRange, size_t, int16_t>> projectedCircles;
        for (const auto &circle: circles) {
            CollisionUtil::CollisionEnvironment env(vpMatrix, is3d, temp1, temp2, halfWidth, halfHeight, sinNegGridAngle, cosNegGridAngle, origin);
            auto projectedCircle = CollisionUtil::getProjectedCircle(circle, env);
            if (!projectedCircle) {
                return 2; // Fully outside of bounds - not relevant
            }
            IndexRange indexRange = getIndexRangeForCircle(*projectedCircle);
            if (indexRange.isValid(numCellsX - 1, numCellsY - 1)) {
                projectedCircles.emplace_back(*projectedCircle, indexRange, circle.contentHash, circle.symbolSpacing);
            }
        }

        if (projectedCircles.empty()) {
            // no valid IndexRanges
            return 2;
        }

        for (const auto &[projectedCircle, indexRange, contentHash, symbolSpacing]: projectedCircles) {

            if (contentHash != 0 && symbolSpacing > 0) {
                const auto &equalRects = spacedRects.find(contentHash);
                if (equalRects != spacedRects.end()) {
                    for (const auto &other: equalRects->second) {
                        // Assume equal symbol spacing for all primitives with matching content
                        if (CollisionUtil::checkRectCircleCollision(other, projectedCircle, symbolSpacing)) {
                            return 1;

                        }
                    }
                }
                const auto &equalCircles = spacedCircles.find(contentHash);
                if (equalCircles != spacedCircles.end()) {
                    for (const auto &other: equalCircles->second) {
                        // Assume equal symbol spacing for all primitives with matching content
                        if (CollisionUtil::checkCircleCollision(projectedCircle, other, symbolSpacing)) {
                            return 1;
                        }
                    }
                }
            }
        }

        if (alwaysInsert) {
            return checkCirclesInsertAlways(circles, projectedCircles);
        } else {
            return checkCirclesInsertOnCollision(circles, projectedCircles);
        }
    }

private:
    uint8_t checkRectInsertOnCollision(const CollisionRectD &rectangle, const RectD &projectedRectangle, const IndexRange &indexRange) {
        for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
            for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                for (const auto &rect: gridRects[y][x]) {
                    if (CollisionUtil::checkRectCollision(projectedRectangle, rect)) {
                        return 1;
                    }
                }
                for (const auto &circle: gridCircles[y][x]) {
                    if (CollisionUtil::checkRectCircleCollision(projectedRectangle, circle)) {
                        return 1;
                    }
                }
            }
        }

        for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
            for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                gridRects[y][x].emplace_back(projectedRectangle);
            }
        }
        if (rectangle.contentHash != 0 && rectangle.symbolSpacing > 0) {
            spacedRects[rectangle.contentHash].push_back(projectedRectangle);
        }

        return 0;
    }

    uint8_t checkRectInsertAlways(const CollisionRectD &rectangle, const RectD &projectedRectangle, const IndexRange &indexRange) {
        bool colliding = false;
        for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
            for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                if (!colliding) {
                    for (const auto &rect: gridRects[y][x]) {
                        if (CollisionUtil::checkRectCollision(projectedRectangle, rect)) {
                            colliding = true;
                        }
                    }
                    for (const auto &circle: gridCircles[y][x]) {
                        if (CollisionUtil::checkRectCircleCollision(projectedRectangle, circle)) {
                            colliding = true;
                        }
                    }
                }
                gridRects[y][x].emplace_back(projectedRectangle);
            }
        }

        if (rectangle.contentHash != 0 && rectangle.symbolSpacing > 0) {
            spacedRects[rectangle.contentHash].push_back(projectedRectangle);
        }

        return colliding ? 1 : 0;
    }

    uint8_t checkCirclesInsertOnCollision(const std::vector<CollisionCircleD> &circles, const std::vector<std::tuple<CircleD, IndexRange, size_t, int16_t>> &projectedCircles) {
        for (const auto &[projectedCircle, indexRange, contentHash, symbolSpacing] : projectedCircles) {
            for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
                for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                    for (const auto &rect : gridRects[y][x]) {
                        if (CollisionUtil::checkRectCircleCollision(rect, projectedCircle)) {
                            return 1;
                        }
                    }
                    for (const auto &circle : gridCircles[y][x]) {
                        if (CollisionUtil::checkCircleCollision(projectedCircle, circle)) {
                            return 1;
                        }
                    }
                }
            }
        }

        for (const auto &[projectedCircle, indexRange, contentHash, symbolSpacing]: projectedCircles) {
            for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
                for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                    gridCircles[y][x].push_back(projectedCircle);
                }
            }
            if (contentHash != 0 && symbolSpacing > 0) {
                spacedCircles[contentHash].push_back(projectedCircle);
            }
        }

        return 0;
    }

    uint8_t checkCirclesInsertAlways(const std::vector<CollisionCircleD> &circles, const std::vector<std::tuple<CircleD, IndexRange, size_t, int16_t>> &projectedCircles) {
        bool colliding = false;
        for (const auto &[projectedCircle, indexRange, contentHash, symbolSpacing]: projectedCircles) {
            for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
                for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                    if (!colliding) {
                        for (const auto &rect: gridRects[y][x]) {
                            if (CollisionUtil::checkRectCircleCollision(rect, projectedCircle)) {
                                colliding = true;
                            }
                        }
                        for (const auto &circle: gridCircles[y][x]) {
                            if (CollisionUtil::checkCircleCollision(projectedCircle, circle)) {
                                colliding = true;
                            }
                        }
                    }
                    gridCircles[y][x].push_back(projectedCircle);
                }
            }
            if (contentHash != 0 && symbolSpacing > 0) {
                spacedCircles[contentHash].push_back(projectedCircle);
            }
        }

        return colliding ? 1 : 0;
    }

    /**
     * Get index range for a projected rectangle
     */
    IndexRange getIndexRangeForRectangle(const RectD &rectangle) {
        IndexRange result;
        result.addXIndex(std::floor(std::clamp(rectangle.x / cellSize, limitLow, limitHigh)) + numCellsPadding, numCellsX - 1);
        result.addXIndex(std::floor(std::clamp(rectangle.x + rectangle.width, limitLow, limitHigh) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addYIndex(std::floor(std::clamp(rectangle.y / cellSize, limitLow, limitHigh)) + numCellsPadding, numCellsY - 1);
        result.addYIndex(std::floor(std::clamp(rectangle.y + rectangle.height, limitLow, limitHigh) / cellSize) + numCellsPadding, numCellsY - 1);
        return result;
    }

    /**
     * Get index range for a projected circle
     */
    IndexRange getIndexRangeForCircle(const CircleD &circle) {
        IndexRange result;
        // May include unnecessary corner grid cells
        result.addXIndex(std::floor(std::clamp(circle.x - circle.radius, limitLow, limitHigh) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addXIndex(std::floor(std::clamp(circle.x + circle.radius, limitLow, limitHigh) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addYIndex(std::floor(std::clamp(circle.y - circle.radius, limitLow, limitHigh) / cellSize) + numCellsPadding, numCellsY - 1);
        result.addYIndex(std::floor(std::clamp(circle.y + circle.radius, limitLow, limitHigh) / cellSize) + numCellsPadding, numCellsY - 1);
        return result;
    }

    // TODO: use smart calculations to define grid number on initialize (e.g. with first insertion o.s.)
    const static int16_t numCellsMinDim = 20;
    // Additional cell padding around the viewport;
    static constexpr int16_t numCellsPadding = 4;

    static constexpr double limitLow = std::numeric_limits<int16_t>::min();
    static constexpr double limitHigh = std::numeric_limits<int16_t>::max();

    const std::vector<double> vpMatrix;
    const Vec2I size;
    const double sinNegGridAngle, cosNegGridAngle;
    double cellSize;
    int16_t numCellsX;
    int16_t numCellsY;
    double halfWidth;
    double halfHeight;
public:
    std::vector<std::vector<std::vector<RectD>>> gridRects; // vector of rectangles in a 2-dimensional gridRects[y][x]
    std::vector<std::vector<std::vector<CircleD>>> gridCircles; // vector of circles in a 2-dimensional gridCircles[y][x]
    std::unordered_map<size_t, std::vector<RectD>> spacedRects;
    std::unordered_map<size_t, std::vector<CircleD>> spacedCircles;

    bool alwaysInsert = false;
    bool is3d;

    const Vec3D origin;

    std::vector<double> temp1 = {0, 0, 0, 0}, temp2 = {0, 0, 0, 0};
};
