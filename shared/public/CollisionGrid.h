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
#include "RectF.h"
#include "CircleF.h"
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
        numCellsX = std::ceil(size.x / cellSize) + 2 * numCellsPadding;
        numCellsY = std::ceil(size.y / cellSize) + 2 * numCellsPadding;
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
    bool addAndCheckCollisionAlignedRect(const CollisionRectF &rectangle) {
        RectF projectedRectangle = getProjectedRectangle(rectangle);
        IndexRange indexRange = getIndexRangeForRectangle(projectedRectangle);
        if (!indexRange.isValid(numCellsX - 1, numCellsY - 1)) {
            return true; // Fully outside of bounds - not relevant
        }

        if (rectangle.contentHash != 0 && rectangle.symbolSpacing > 0) {
            const auto &equalRects = spacedRects.find(rectangle.contentHash);
            if (equalRects != spacedRects.end()) {
                for (const auto &other : equalRects->second) {
                    // Assume equal symbol spacing for all primitives with matching content
                    if (checkRectCollision(projectedRectangle, other, rectangle.symbolSpacing)) {
                        return true;
                    }
                }
            }
            const auto &equalCircles = spacedCircles.find(rectangle.contentHash);
            if (equalCircles != spacedCircles.end()) {
                for (const auto &other : equalCircles->second) {
                    // Assume equal symbol spacing for all primitives with matching content
                    if (checkRectCircleCollision(projectedRectangle, other, rectangle.symbolSpacing)) {
                        return true;
                    }
                }
            }
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
        for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
            for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                gridRects[y][x].push_back(projectedRectangle);
            }
        }

        if (rectangle.contentHash != 0 && rectangle.symbolSpacing > 0) {
            spacedRects[rectangle.contentHash].push_back(projectedRectangle);
        }

        return false;
    }

    /**
    * Add a vector of circles (which are then projected with the provided vpMatrix) and receive the feedback, if they have collided
    * with the previous content of the grid. Assumed to remain circles in the projected space. Only added, when not colliding!
    */
    bool addAndCheckCollisionCircles(const std::vector<CollisionCircleF> &circles) {
        if (circles.empty()) {
            // No circles -> no collision
            return false;
        }

        std::vector<std::tuple<CircleF, IndexRange, size_t, int32_t>> projectedCircles;
        for (const auto &circle : circles) {
            auto projectedCircle = getProjectedCircle(circle);
            IndexRange indexRange = getIndexRangeForCircle(projectedCircle);
            if (indexRange.isValid(numCellsX - 1, numCellsY - 1)) {
                projectedCircles.emplace_back(projectedCircle, indexRange, circle.contentHash, circle.symbolSpacing);
            }
        }

        if (projectedCircles.empty()) {
            // no valid IndexRanges
            return true;
        }

        for (const auto &[projectedCircle, indexRange, contentHash, symbolSpacing] : projectedCircles) {

            if (contentHash != 0 && symbolSpacing > 0) {
                const auto &equalRects = spacedRects.find(contentHash);
                if (equalRects != spacedRects.end()) {
                    for (const auto &other : equalRects->second) {
                        // Assume equal symbol spacing for all primitives with matching content
                        if (checkRectCircleCollision(other, projectedCircle, symbolSpacing)) {
                            return true;
                        }
                    }
                }
                const auto &equalCircles = spacedCircles.find(contentHash);
                if (equalCircles != spacedCircles.end()) {
                    for (const auto &other : equalCircles->second) {
                        // Assume equal symbol spacing for all primitives with matching content
                        if (checkCircleCollision(projectedCircle, other, symbolSpacing)) {
                            return true;
                        }
                    }
                }
            }

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
        for (const auto &[projectedCircle, indexRange, contentHash, symbolSpacing] : projectedCircles) {
            for (int16_t y = indexRange.yMin; y <= indexRange.yMax; y++) {
                for (int16_t x = indexRange.xMin; x <= indexRange.xMax; x++) {
                    gridCircles[y][x].push_back(projectedCircle);
                }
            }
            if (contentHash != 0 && symbolSpacing > 0) {
                spacedCircles[contentHash].push_back(projectedCircle);
            }
        }
        return false;
    }

private:
    RectF getProjectedRectangle(const CollisionRectF &rectangle) {
        temp2[0] = rectangle.x;
        temp2[1] = rectangle.y;
        temp2[2] = 0.0;
        temp2[3] = 1.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        float originX = ((temp1.at(0) / temp1.at(3)) * halfWidth + halfWidth);
        float originY = ((temp1.at(1) / temp1.at(3)) * halfHeight + halfHeight);
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
        float width = (w * halfWidth); // by assumption aligned with projected space
        float height = (h * halfHeight); // by assumption aligned with projected space
        originX = std::min(originX, originX + width);
        originY = std::min(originY, originY + height);
        // Rectangle origin is chosen as the min/min corner with width/height always positive
        return {originX, originY, std::abs(width), std::abs(height)};
    }

    CircleF getProjectedCircle(const CollisionCircleF &circle) {
        temp2[0] = circle.x;
        temp2[1] = circle.y;
        temp2[2] = 0.0;
        temp2[3] = 1.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        float originX = ((temp1.at(0) / temp1.at(3)) * halfWidth + halfWidth);
        float originY = ((temp1.at(1) / temp1.at(3)) * halfHeight + halfHeight);
        temp2[0] = circle.radius;
        temp2[1] = circle.radius;
        temp2[2] = 0.0;
        temp2[3] = 0.0;
        Matrix::multiply(vpMatrix, temp2, temp1);
        temp1.at(0) = temp1.at(0) * halfWidth;
        temp1.at(1) = temp1.at(1) * halfHeight;
        float iRadius = std::abs((std::sqrt(temp1.at(0) * temp1.at(0) + temp1.at(1) + temp1.at(1))));
        return {originX, originY, iRadius};
    }

    /**
     * Get index range for a projected rectangle
     */
    IndexRange getIndexRangeForRectangle(const RectF &rectangle) {
        IndexRange result;
        result.addXIndex(std::floor((rectangle.x - collisionBias) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addXIndex(std::floor((rectangle.x + rectangle.width + collisionBias) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addYIndex(std::floor((rectangle.y - collisionBias) / cellSize) + numCellsPadding, numCellsY - 1);
        result.addYIndex(std::floor((rectangle.y + rectangle.height + collisionBias) / cellSize) + numCellsPadding, numCellsY - 1);
        return result;
    }

    /**
     * Get index range for a projected circle
     */
    IndexRange getIndexRangeForCircle(const CircleF &circle) {
        IndexRange result;
        // May include unnecessary corner grid cells
        result.addXIndex(std::floor((circle.x - circle.radius - collisionBias) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addXIndex(std::floor((circle.x + circle.radius + collisionBias) / cellSize) + numCellsPadding, numCellsX - 1);
        result.addYIndex(std::floor((circle.y - circle.radius - collisionBias) / cellSize) + numCellsPadding, numCellsY - 1);
        result.addYIndex(std::floor((circle.y + circle.radius + collisionBias) / cellSize) + numCellsPadding, numCellsY - 1);
        return result;
    }

    bool checkRectCollision(const RectF &rect1, const RectF &rect2, int32_t addSpacing = 0) {
        return (rect1.x < (rect2.x + rect2.width + addSpacing + collisionBias)) &&
               ((rect1.x + rect1.width) > (rect2.x - addSpacing - collisionBias)) &&
               (rect1.y < (rect2.y + rect2.height + addSpacing + collisionBias)) &&
               ((rect1.y + rect1.height) > (rect2.y - addSpacing - collisionBias));
    }

    bool checkRectCircleCollision(const RectF &rect, const CircleF &circle, int32_t addSpacing = 0) {
        float minX = std::min(rect.x + rect.width, rect.x);
        float minY = std::min(rect.y + rect.height, rect.y);
        float closestX = std::max(minX, std::min(minX + rect.width, circle.x));
        float closestY = std::max(minY, std::min(minY + rect.height, circle.y));
        float dX = closestX - circle.x;
        float dY = closestY - circle.y;
        float r = circle.radius + addSpacing + collisionBias;
        return (dX * dX + dY * dY) < (r * r);
    }

    bool checkCircleCollision(const CircleF &circle1, const CircleF &circle2, int32_t addSpacing = 0) {
        float dX = circle1.x - circle2.x;
        float dY = circle1.y - circle2.y;
        float distanceSq = dX * dX + dY * dY;
        float r = circle1.radius + circle2.radius + addSpacing + collisionBias;
        return distanceSq < (r * r);
    }

    const static int32_t numCellsMinDim = 20; // TODO: use smart calculations to define grid number on initialize (e.g. with first insertion o.s.)
    // Additional cell padding around the viewport;
    static constexpr int32_t numCellsPadding = 4;

    const std::vector<float> vpMatrix;
    const Vec2I size;
    const float sinNegGridAngle, cosNegGridAngle;
    float cellSize;
    int16_t numCellsX;
    int16_t numCellsY;
    float halfWidth;
    float halfHeight;
    std::vector<std::vector<std::vector<RectF>>> gridRects; // vector of rectangles in a 2-dimensional gridRects[y][x]
    std::vector<std::vector<std::vector<CircleF>>> gridCircles; // vector of circles in a 2-dimensional gridCircles[y][x]
    std::unordered_map<size_t, std::vector<RectF>> spacedRects;
    std::unordered_map<size_t, std::vector<CircleF>> spacedCircles;

    // Additional padding to the advantage of established collision primitives to resolve any rounding errors
    static constexpr int32_t collisionBias = 0;

    std::vector<float> temp1 = {0, 0, 0, 0}, temp2 = {0, 0, 0, 0};
};