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

#include "Vec2I.h"
#include "RectD.h"
#include "RectI.h"
#include "CircleD.h"
#include "CircleI.h"
#include "Matrix.h"
#include <vector>

class IndexRange {
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

    bool isValid(int16_t maxX, int16_t maxY) {
        return xMin <= maxX && xMax >= 0 && yMin <= maxY && yMax >= 0;
    }
};

class CollisionGrid {
public:
    CollisionGrid(const std::vector<float> &vpMatrix, const Vec2I &size, float gridAngle)
            : vpMatrix(vpMatrix), size(size),
              sinNegGridAngle(std::sin(-gridAngle * M_PI / 180.0)),
              cosNegGridAngle(std::cos(-gridAngle * M_PI / 180.0)) {
        cellSize = std::min(size.x, size.y) / (float) numCellsMinDim;
        numCellsX = std::ceil(size.x / cellSize);
        numCellsY = std::ceil(size.y / cellSize);
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
     */
    bool addAndCheckCollisionAlignedRect(const RectD &rectangle) {
        RectI projectedRectangle = getProjectedRectangle(rectangle);
        std::optional<IndexRange> indexRange = getIndexRangeForRectangle(projectedRectangle);
        if (!indexRange.has_value()) {
            return true; // Fully outside of bounds - not relevant
        }
        for (int16_t y = indexRange->yMin; y <= indexRange->yMax; y++) {
            for (int16_t x = indexRange->xMin; x <= indexRange->xMax; x++) {
                for (const auto &rect : gridRects[y][x]) {
                    if (checkRectCollision(projectedRectangle, rect)) {
                        return true;
                    }
                }
                for (const auto &circle : gridCircles[y][x]) {
                    if (checkRectCircleCollision(projectedRectangle, circle)) {
                        return true;
                    }
                }
            }
        }
        // Only insert, when not colliding
        for (int16_t y = indexRange->yMin; y <= indexRange->yMax; y++) {
            for (int16_t x = indexRange->xMin; x <= indexRange->xMax; x++) {
                gridRects[y][x].push_back(projectedRectangle);
            }
        }
        return false;
    }

    /**
    * Add a vector of circles (which are then projected with the provided vpMatrix) and receive the feedback, if they have collided
    * with the previous content of the grid. Assumed to remain circles in the projected space. Only added, when not colliding!
    */
    bool addAndCheckCollisionCircles(const std::vector<CircleD> &circles) {
        if (circles.empty()) {
            // No circles -> no collision
            return false;
        }

        std::vector<std::tuple<CircleI, IndexRange>> projectedCircles;
        for (const auto &circle : circles) {
            auto projectedCircle = getProjectedCircle(circle);
            auto indexRange = getIndexRangeForCircle(projectedCircle);
            if (indexRange.has_value()) {
                projectedCircles.emplace_back(projectedCircle, *indexRange);
            }
        }

        if (projectedCircles.empty()) {
            // no valid IndexRanges
            return true;
        }

        for (const auto &[projectedCircle, indexRange] : projectedCircles) {
            for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
                for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                    for (const auto &rect : gridRects[y][x]) {
                        if (checkRectCircleCollision(rect, projectedCircle)) {
                            return true;
                        }
                    }
                    for (const auto &circle : gridCircles[y][x]) {
                        if (checkCircleCollision(projectedCircle, circle)) {
                            return true;
                        }
                    }
                }
            }
        }

        // Only insert when none colliding
        for (const auto &[projectedCircle, indexRange] : projectedCircles) {
            for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
                for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                    gridCircles[y][x].push_back(projectedCircle);
                }
            }
        }
        return false;
    }

private:
    RectI getProjectedRectangle(const RectD &rectangle) {
        std::vector<float> origin(4), width(4), height(4);
        Matrix::multiply(vpMatrix, {(float) rectangle.x, (float) rectangle.y, 0.0, 1.0}, origin);
        Matrix::multiply(vpMatrix, {(float) rectangle.width * cosNegGridAngle, (float) rectangle.width * sinNegGridAngle, 0.0, 0.0}, width);
        Matrix::multiply(vpMatrix, {(float) -rectangle.height * sinNegGridAngle, (float) rectangle.height * cosNegGridAngle, 0.0, 0.0}, height);
        RectI result = RectI(std::round((origin.at(0) / origin.at(3)) * halfWidth + halfWidth),
                     std::round((origin.at(1) / origin.at(3)) * halfHeight + halfHeight),
                     std::round((width.at(0) + height.at(0)) * halfWidth), // by assumption aligned with projected space
                     std::round((width.at(1) + height.at(1)) * halfHeight)); // by assumption aligned with projected space
        return result;
    }

    CircleI getProjectedCircle(const CircleD &circle) {
        std::vector<float> origin(4), radius(4);
        Matrix::multiply(vpMatrix, {(float) circle.origin.x, (float) circle.origin.y, 0.0, 1.0}, origin);
        Matrix::multiply(vpMatrix, {(float) circle.radius, (float) circle.radius, 0.0, 0.0}, radius);
        radius.at(0) = radius.at(0) * halfWidth;
        radius.at(1) = radius.at(1) * halfHeight;
        int32_t iRadius = std::round(std::sqrt(radius.at(0) * radius.at(0) + radius.at(1) + radius.at(1)));
        return {Vec2I(std::round((origin.at(0) / origin.at(3)) * halfWidth + halfWidth),
                      std::round((origin.at(1) / origin.at(3)) * halfHeight + halfHeight)),
                iRadius};
    }

    /**
     * Get index range for a projected rectangle
     */
    std::optional<IndexRange> getIndexRangeForRectangle(const RectI &rectangle) {
        IndexRange result;
        result.addXIndex(std::round(rectangle.x / cellSize), numCellsX - 1);
        result.addXIndex(std::round((rectangle.x + rectangle.width) / cellSize), numCellsX - 1);
        result.addYIndex(std::round(rectangle.y / cellSize), numCellsY - 1);
        result.addYIndex(std::round((rectangle.y + rectangle.height) / cellSize), numCellsY - 1);
        if (result.isValid(numCellsX - 1, numCellsY - 1)) {
            return result;
        } else {
            return std::nullopt;
        }
    }

    /**
     * Get index range for a projected circle
     */
    std::optional<IndexRange> getIndexRangeForCircle(const CircleI &circle) {
        IndexRange result;
        // May include unnecessary grid cell in the corner in certain cases
        result.addXIndex(std::round((circle.origin.x - circle.radius) / cellSize), numCellsX - 1);
        result.addXIndex(std::round((circle.origin.x + circle.radius) / cellSize), numCellsX - 1);
        result.addYIndex(std::round((circle.origin.y - circle.radius) / cellSize), numCellsY - 1);
        result.addYIndex(std::round((circle.origin.y + circle.radius) / cellSize), numCellsY - 1);
        if (result.isValid(numCellsX - 1, numCellsY - 1)) {
            return result;
        } else {
            return std::nullopt;
        }
    }

    bool checkRectCollision(const RectI &rect1, const RectI &rect2) {
        bool collides = std::min(rect1.x, rect1.x + rect1.width) < std::max(rect2.x, rect2.x + rect2.width) &&
                        std::max(rect1.x, rect1.x + rect1.width) > std::min(rect2.x, rect2.x + rect2.width) &&
                        std::min(rect1.y, rect1.y + rect1.height) < std::max(rect2.y, rect2.y + rect2.height) &&
                        std::max(rect1.y, rect1.y + rect1.height) > std::min(rect2.y, rect2.y + rect2.height);
        return collides;
    }

    bool checkRectCircleCollision(const RectI &rect, const CircleI &circle) {
        int32_t minX = std::min(rect.x + rect.width, rect.x);
        int32_t minY = std::min(rect.y + rect.height, rect.y);
        int32_t closestX = std::max(minX, std::min(minX + std::abs(rect.width), circle.origin.x));
        int32_t closestY = std::max(minY, std::min(minY + std::abs(rect.height), circle.origin.y));
        int32_t dX = closestX - circle.origin.x;
        int32_t dY = closestY - circle.origin.y;
        int32_t distanceSq = dX * dX + dY * dY;
        return distanceSq < (circle.radius * circle.radius);
    }

    bool checkCircleCollision(const CircleI &circle1, const CircleI &circle2) {
        int32_t dX = circle1.origin.x - circle2.origin.x;
        int32_t dY = circle1.origin.y - circle2.origin.y;
        int32_t distanceSq = dX * dX + dY * dY;
        int32_t r = circle1.radius + circle2.radius;
        return distanceSq < (r * r);
    }


    const static int32_t numCellsMinDim = 20; // TODO: use smart calculations to define grid number on initialize (e.g. with first insertion o.s.)

    const std::vector<float> vpMatrix;
    const Vec2I size;
    const float sinNegGridAngle, cosNegGridAngle;
    float cellSize;
    int16_t numCellsX;
    int16_t numCellsY;
    float halfWidth;
    float halfHeight;
    std::vector<std::vector<std::vector<RectI>>> gridRects; // vector of rectangles in a 2-dimensional gridRects[y][x]
    std::vector<std::vector<std::vector<CircleI>>> gridCircles; // vector of circles in a 2-dimensional gridCircles[y][x]
};