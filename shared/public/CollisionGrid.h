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
#include "CollisionPrimitives.h"
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
    bool addAndCheckCollisionAlignedRect(const CollisionRectD &rectangle) {
        CollisionRectI projectedRectangle = getProjectedRectangle(rectangle);
        IndexRange indexRange = getIndexRangeForRectangle(projectedRectangle, false);
        if (!indexRange.isValid(numCellsX - 1, numCellsY - 1)) {
            return true; // Fully outside of bounds - not relevant
        }
        for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
            for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
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
        if (projectedRectangle.symbolSpacing > 0) {
            indexRange = getIndexRangeForRectangle(projectedRectangle, true);
        }
        for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
            for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                gridRects[y][x].push_back(projectedRectangle);
            }
        }
        return false;
    }

    /**
    * Add a vector of circles (which are then projected with the provided vpMatrix) and receive the feedback, if they have collided
    * with the previous content of the grid. Assumed to remain circles in the projected space. Only added, when not colliding!
    */
    bool addAndCheckCollisionCircles(const std::vector<CollisionCircleD> &circles) {
        if (circles.empty()) {
            // No circles -> no collision
            return false;
        }

        std::vector<std::tuple<CollisionCircleI, IndexRange>> projectedCircles;
        for (const auto &circle : circles) {
            auto projectedCircle = getProjectedCircle(circle);
            IndexRange indexRange = getIndexRangeForCircle(projectedCircle, false);
            if (indexRange.isValid(numCellsX - 1, numCellsY - 1)) {
                projectedCircles.emplace_back(projectedCircle, indexRange);
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
        IndexRange insertionRange;
        for (const auto &[projectedCircle, indexRange] : projectedCircles) {
            insertionRange = projectedCircle.symbolSpacing > 0 ? getIndexRangeForCircle(projectedCircle, true) : indexRange;
            for (int16_t y = insertionRange.yMin; y <= insertionRange.yMax; y++) {
                for (int16_t x = insertionRange.xMin; x <= insertionRange.xMax; x++) {
                    gridCircles[y][x].push_back(projectedCircle);
                }
            }
        }
        return false;
    }

private:
    CollisionRectI getProjectedRectangle(const CollisionRectD &rectangle) {
        temp2[0] = rectangle.x;
        temp2[1] = rectangle.y;
        temp2[2] = 0.0;
        temp2[3] = 1.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        int32_t originX = std::round((temp1.at(0) / temp1.at(3)) * halfWidth + halfWidth);
        int32_t originY = std::round((temp1.at(1) / temp1.at(3)) * halfHeight + halfHeight);
        temp2[0] = rectangle.width * cosNegGridAngle;
        temp2[1] = rectangle.width * sinNegGridAngle;
        temp2[2] = 0.0;
        temp2[3] = 0.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        float w = temp1.at(0);
        float h = temp1.at(1);
        temp2[0] = -rectangle.height * sinNegGridAngle;
        temp2[1] = rectangle.height * cosNegGridAngle;
        temp2[2] = 0.0;
        temp2[3] = 0.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        w += temp1.at(0);
        h += temp1.at(1);
        int32_t width = std::round(w * halfWidth);
        int32_t height = std::round(h * halfHeight);
        originX = std::min(originX, originX + width);
        originY = std::min(originY, originY + height);
        return {originX, originY, std::abs(width), std::abs(height), // by assumption aligned with projected space
                rectangle.contentHash, (int32_t) std::round(rectangle.symbolSpacing)};
    }

    CollisionCircleI getProjectedCircle(const CollisionCircleD &circle) {
        temp2[0] = circle.x;
        temp2[1] = circle.y;
        temp2[2] = 0.0;
        temp2[3] = 1.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        int32_t originX = std::round((temp1.at(0) / temp1.at(3)) * halfWidth + halfWidth);
        int32_t originY = std::round((temp1.at(1) / temp1.at(3)) * halfHeight + halfHeight);
        temp2[0] = circle.radius;
        temp2[1] = circle.radius;
        temp2[2] = 0.0;
        temp2[3] = 0.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        temp1.at(0) = temp1.at(0) * halfWidth;
        temp1.at(1) = temp1.at(1) * halfHeight;
        int32_t iRadius = std::abs(std::round(std::sqrt(temp1.at(0) * temp1.at(0) + temp1.at(1) + temp1.at(1))));
        return {originX, originY, iRadius, circle.contentHash, (int32_t) std::round(circle.symbolSpacing)};
    }

    /**
     * Get index range for a projected rectangle
     */
    IndexRange getIndexRangeForRectangle(const CollisionRectI &rectangle, bool includeSpacing) {
        IndexRange result;
        int32_t addSpacing = includeSpacing ? rectangle.symbolSpacing : 0;
        result.addXIndex(std::floor(rectangle.x - addSpacing / cellSize), numCellsX - 1);
        result.addXIndex(std::floor((rectangle.x + rectangle.width + addSpacing) / cellSize), numCellsX - 1);
        result.addYIndex(std::floor(rectangle.y - addSpacing / cellSize), numCellsY - 1);
        result.addYIndex(std::floor((rectangle.y + rectangle.height + addSpacing) / cellSize), numCellsY - 1);
        return result;
    }

    /**
     * Get index range for a projected circle
     */
    IndexRange getIndexRangeForCircle(const CollisionCircleI &circle, bool includeSpacing) {
        IndexRange result;
        // May include unnecessary corner grid cells
        int32_t fullRadius = circle.radius + (includeSpacing ? circle.symbolSpacing : 0);
        result.addXIndex(std::floor((circle.x - fullRadius) / cellSize), numCellsX - 1);
        result.addXIndex(std::floor((circle.x + fullRadius) / cellSize), numCellsX - 1);
        result.addYIndex(std::floor((circle.y - fullRadius) / cellSize), numCellsY - 1);
        result.addYIndex(std::floor((circle.y + fullRadius) / cellSize), numCellsY - 1);
        return result;
    }

    bool checkRectCollision(const CollisionRectI &rect1, const CollisionRectI &rect2) {
        int addPadding = rect1.contentHash != 0 && rect1.contentHash == rect2.contentHash ? rect2.symbolSpacing : 0; // assume equal spacing for equal content
        bool collides = std::min(rect1.x, rect1.x + rect1.width) < std::max(rect2.x + addPadding, rect2.x + rect2.width + addPadding) &&
                        std::max(rect1.x, rect1.x + rect1.width) > std::min(rect2.x - addPadding, rect2.x + rect2.width - addPadding) &&
                        std::min(rect1.y, rect1.y + rect1.height) < std::max(rect2.y + addPadding, rect2.y + rect2.height + addPadding) &&
                        std::max(rect1.y, rect1.y + rect1.height) > std::min(rect2.y - addPadding, rect2.y + rect2.height - addPadding);
        return collides;
    }

    bool checkRectCircleCollision(const CollisionRectI &rect, const CollisionCircleI &circle) {
        int addPadding = rect.contentHash != 0 && rect.contentHash == circle.contentHash ? rect.symbolSpacing : 0; // assume equal spacing for equal content
        int32_t minX = std::min(rect.x + rect.width, rect.x);
        int32_t minY = std::min(rect.y + rect.height, rect.y);
        int32_t closestX = std::max(minX, std::min(minX + std::abs(rect.width), circle.x));
        int32_t closestY = std::max(minY, std::min(minY + std::abs(rect.height), circle.y));
        int32_t dX = closestX - circle.x;
        int32_t dY = closestY - circle.y;
        int32_t distanceSq = dX * dX + dY * dY;
        return distanceSq < ((circle.radius + addPadding) * (circle.radius + addPadding));
    }

    bool checkCircleCollision(const CollisionCircleI &circle1, const CollisionCircleI &circle2) {
        int addPadding = circle1.contentHash != 0 && circle1.contentHash == circle2.contentHash ? circle1.symbolSpacing : 0; // assume equal spacing for equal content
        int32_t dX = circle1.x - circle2.x;
        int32_t dY = circle1.y - circle2.y;
        int32_t distanceSq = dX * dX + dY * dY;
        int32_t r = circle1.radius + circle2.radius + addPadding;
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
    std::vector<std::vector<std::vector<CollisionRectI>>> gridRects; // vector of rectangles in a 2-dimensional gridRects[y][x]
    std::vector<std::vector<std::vector<CollisionCircleI>>> gridCircles; // vector of circles in a 2-dimensional gridCircles[y][x]

    std::vector<float> temp1 = {0, 0, 0, 0}, temp2 = {0, 0, 0, 0};
};