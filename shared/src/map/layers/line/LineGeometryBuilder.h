//
//  LineGeometryBuilder.h
//  MapCore
//
//  Created by Nicolas Märki on 26.05.2025.
//

#include "Vec3DHelper.h"

class LineGeometryBuilder {
  public:
    static void buildLines(const std::shared_ptr<LineGroup2dInterface> &line,
                           const std::vector<std::tuple<std::vector<Vec3D>, int>> &lines, const Vec3D &origin, LineCapType capType,
                           LineJoinType defaultJoinType, bool is3d, bool optimizeForDots) {

        if (optimizeForDots) {
            defaultJoinType = LineJoinType::ROUND; // Force miter join for dot optimization
            capType = LineCapType::ROUND; // Force round cap for dot optimization
        }

        std::vector<float> lineAttributes;
        std::vector<uint32_t> lineIndices;
        reserveEstimatedNumVertices(lines, defaultJoinType, capType, is3d, lineAttributes, lineIndices);

        uint32_t vertexCount = 0;
        int numLines = (int)lines.size();
        for (int lineIndex = numLines - 1; lineIndex >= 0; lineIndex--) {
            auto const &[renderCoords, lineStyleIndex] = lines[lineIndex];

            int pointCount = (int)renderCoords.size();

            if (pointCount < 2) {
                continue; // Skip lines with less than 2 points
            }

            float prefixTotalLineLength = 0.0;
            float lineLength = 0;
            float cosAngle = 0, cosHalfAngle = 0;

            Vec3D pLast(0, 0, 0), normal(0, 0, 0), lastNormal(0, 0, 0), lineVec(0, 0, 0), lastLineVec(0, 0, 0),
                pOriginLine(0, 0, 0);

            int32_t preIndex = -1, prePreIndex = -1;

            for (int currentIndex = 0, nextIndex = 0; currentIndex < pointCount; currentIndex = nextIndex) {
                const Vec3D &p = renderCoords[currentIndex];

                prefixTotalLineLength += lineLength;

                float unlimitedExtrudeScale = 1.0;
                float outerExtrudeScale = 1.0;
                LineJoinType actualJoinType = defaultJoinType;

                float turnDirection = 0;

                Vec3D extrude(0, 0, 0), extrudeMirror(0, 0, 0), extrudeMirrorLast(0, 0, 0), extrudeLineVec(0, 0, 0);

                if (currentIndex > 0) {
                    lastNormal = normal;
                    lastLineVec = lineVec;
                }

                // search for next point with non-zero length vector from current point
                lineLength = 0;
                while (lineLength == 0) {
                    nextIndex++;
                    if (nextIndex >= pointCount) {
                        break;
                    }
                    lineVec = renderCoords[nextIndex] - p;
                    lineLength = Vec3DHelper::length(lineVec);
                }

                if (lineLength > 0 && nextIndex < pointCount) {

                    lineVec /= lineLength;

                    if (is3d) {
                        pOriginLine = Vec3DHelper::normalize(Vec3D(p.x + origin.x, p.y + origin.y, p.z + origin.z));
                        normal = Vec3DHelper::normalize(Vec3DHelper::crossProduct(pOriginLine, lineVec));
                    } else {
                        normal = Vec3DHelper::normalize(Vec3D(-lineVec.y, lineVec.x, 0.0));
                    }

                    if (currentIndex > 0) {
                        extrude = (normal + lastNormal);
                        extrudeMirrorLast = extrude - 2.0 * (lastLineVec * (Vec3DHelper::dotProduct(extrude, lastLineVec)));
                        extrudeMirror = extrude - 2.0 * (lineVec * (Vec3DHelper::dotProduct(extrude, lineVec)));

                        double extrudeLength = Vec3DHelper::length(extrude);
                        if (extrudeLength > 0) {
                            extrude /= extrudeLength;
                        }
                        double extrudeMirrorLastLength = Vec3DHelper::length(extrudeMirrorLast);
                        if (extrudeMirrorLastLength > 0) {
                            extrudeMirrorLast /= extrudeMirrorLastLength;
                        }
                        double extrudeMirrorLength = Vec3DHelper::length(extrudeMirror);
                        if (extrudeMirrorLength > 0) {
                            extrudeMirror /= extrudeMirrorLength;
                        }

                        if (is3d) {
                            cosAngle = Vec3DHelper::dotProduct(normal, lastNormal);
                            cosHalfAngle = Vec3DHelper::dotProduct(extrude, normal);

                            Vec3D cross = Vec3DHelper::crossProduct(lastLineVec, lineVec); // 3D cross product
                            turnDirection = Vec3DHelper::dotProduct(cross, pOriginLine);   // Project onto reference normal

                        } else {
                            cosAngle = normal.x * lastNormal.x + normal.y * lastNormal.y;
                            cosHalfAngle = extrude.x * normal.x + extrude.y * normal.y;
                            turnDirection = (lastNormal.x * normal.y - lastNormal.y * normal.x); // 2D cross product
                        }

                        unlimitedExtrudeScale = cosHalfAngle != 0 ? std::abs(1.0 / cosHalfAngle) : 1.0;
                        outerExtrudeScale = unlimitedExtrudeScale;

                        if (defaultJoinType == LineJoinType::MITER && unlimitedExtrudeScale > 2.0) {
                            actualJoinType = LineJoinType::BEVEL;
                            outerExtrudeScale = 2.0; // Limit the outer miter, but keep unlimitedExtrudeScale for the inner corner.
                        }
                    } else {
                        extrude = normal;
                        extrudeLineVec = lineVec;
                    }
                } else if (currentIndex > 0) {
                    // last point
                    extrude = lastNormal;
                    extrudeLineVec = -lastLineVec;
                } else {
                    lineLength = 0;
                    continue;
                }

                float endSide = 0;
                if (currentIndex == 0) {
                    endSide = -1;
                } else if (currentIndex == pointCount - 1) {
                    endSide = 1;
                }

                if (endSide == -1 && capType == LineCapType::ROUND) {
                    auto originalPrePreIndex = prePreIndex;
                    auto originalPreIndex = preIndex;
                    float prefixCorrection = 0.0;
                    pushLineVertex(p, Vec3D(0, 0, 0), 1.0, 0, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, false, vertexCount,
                                   prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                    int32_t centerIndex = preIndex, firstIndex = -1, lastIndex = -1;
                    for (float r = -1; r <= 1; r += 0.2) {
                        Vec3D roundExtrude = Vec3DHelper::normalize(extrude * r - extrudeLineVec * (1.0 - std::abs(r)));
                        prefixCorrection = Vec3DHelper::dotProduct(roundExtrude, lastLineVec);
                        pushLineVertex(p, roundExtrude, 1.0, r, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, endSide == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                        if (r == 0) {
                            firstIndex = preIndex;
                        } else {
                            lastIndex = preIndex;
                        }
                        prePreIndex = centerIndex;
                    }
                    prePreIndex = originalPrePreIndex;
                    preIndex = originalPreIndex;
                }

                for (int8_t side = -1; side <= 1; side += 2) {
                    Vec3D pointExtrude = extrude * (double)side;
                    if (capType == LineCapType::SQUARE && endSide != 0) {
                        pointExtrude = pointExtrude + extrudeLineVec * endSide;
                    }
                    if (side * turnDirection < 0 && endSide == 0) { // This is the INNER ("mirrored") side of the turn
                        float prefixCorrection = Vec3DHelper::dotProduct(extrudeMirrorLast * (double)side * unlimitedExtrudeScale, lastLineVec);
                        pushLineVertex(p, extrudeMirrorLast * (double)side, unlimitedExtrudeScale, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, side == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                    } else { 
                        float prefixCorrection = Vec3DHelper::dotProduct(pointExtrude * unlimitedExtrudeScale, lastLineVec);
                        pushLineVertex(p, pointExtrude, unlimitedExtrudeScale, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, side == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                    }
                }

                for (int8_t side = -1; side <= 1; side += 2) {
                    Vec3D pointExtrude = extrude * (double)side;
                    if (capType == LineCapType::SQUARE && endSide != 0) {
                        pointExtrude = pointExtrude + extrudeLineVec * endSide;
                    }
                    if (side * turnDirection < 0 && endSide == 0) {
                        bool shouldRound = unlimitedExtrudeScale > 1.1 || optimizeForDots;
                        if (shouldRound && actualJoinType != LineJoinType::MITER) {
                            const double approxAngle = 2 * std::sqrt(2 - 2 * cosHalfAngle);
                            // 2.86 ~= 180/pi / 20 -> approximately one slice per 20 degrees
                            const int stepCount = (actualJoinType == LineJoinType::ROUND) ? std::max(1.0, round(approxAngle * 1.46)) : 1;
                            for (int step = 0; step <= stepCount; step++) {
                                double r = (double)step / (double)stepCount * 0.5;
                                pointExtrude = Vec3DHelper::normalize(lastNormal * (1.0 - r) + normal * r) * (double)side;
                                if (side == 1) {
                                    std::swap(prePreIndex, preIndex);
                                }
                                float prefixCorrection = Vec3DHelper::dotProduct(pointExtrude, lastLineVec);
                                pushLineVertex(p, pointExtrude, 1.0, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, side == -1,
                                               vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                                if (side == -1) {
                                    std::swap(prePreIndex, preIndex);
                                }
                            }
                            float prefixCorrection = Vec3DHelper::dotProduct(extrude * (double)-side * unlimitedExtrudeScale, lineVec);
                            pushLineVertex(p, extrude * (double)-side, unlimitedExtrudeScale, -side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, false, -side == -1,
                                           vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                            if (side == 1) {
                                std::swap(prePreIndex, preIndex);
                            }
                            for (int step = 0; step <= stepCount; step++) {
                                double r = (double)step / (double)stepCount * 0.5 + 0.5;
                                pointExtrude = Vec3DHelper::normalize(lastNormal * (1.0 - r) + normal * r) * (double)side;
                                if (side == 1) {
                                    std::swap(prePreIndex, preIndex);
                                }
                                float prefixCorrection = Vec3DHelper::dotProduct(pointExtrude, lineVec);
                                pushLineVertex(p, pointExtrude, 1.0, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, side == -1,
                                               vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                                if (side == -1) {
                                    std::swap(prePreIndex, preIndex);
                                }
                            }
                            if (side == 1) {
                                std::swap(prePreIndex, preIndex);
                            }
                        }
                        else {
                            if (side == 1) {
                                std::swap(prePreIndex, preIndex);
                            }
                            float prefixCorrection = Vec3DHelper::dotProduct(extrude * (double)side, lastLineVec);
                            pushLineVertex(p, extrude * (double)side, 1.0, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, side == -1,
                                           vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                            std::swap(prePreIndex, preIndex);
                            prefixCorrection = Vec3DHelper::dotProduct(extrude * (double)side, lineVec);
                            pushLineVertex(p, extrude * (double)side, 1.0, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, false, side == -1,
                                           vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                            std::swap(prePreIndex, preIndex);
                        }

                        float prefixCorrection = Vec3DHelper::dotProduct(extrudeMirror * (double)side * unlimitedExtrudeScale, lineVec);
                        pushLineVertex(p, extrudeMirror * (double)side, unlimitedExtrudeScale, side, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, side == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                        if (side == -1) {
                            std::swap(prePreIndex, preIndex);
                        }
                    }


                }

                if (endSide == 1 && capType == LineCapType::ROUND) {
                    auto originalPrePreIndex = prePreIndex;
                    auto originalPreIndex = preIndex;
                    float prefixCorrection = 0;
                    pushLineVertex(p, Vec3D(0, 0, 0), 1.0, 0, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, false, vertexCount,
                                   prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                    int32_t centerIndex = preIndex, firstIndex = -1, lastIndex = -1;
                    for (float r = -1; r <= 1; r += 0.2) {
                        Vec3D roundExtrude = Vec3DHelper::normalize(extrude * r - extrudeLineVec * (1.0 - std::abs(r)));
                        float prefixCorrection = Vec3DHelper::dotProduct(roundExtrude, lineVec);
                        pushLineVertex(p, roundExtrude, 1.0, r, prefixTotalLineLength, prefixCorrection, lineStyleIndex, true, endSide == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices, is3d);
                        if (r == 0) {
                            firstIndex = preIndex;
                        } else {
                            lastIndex = preIndex;
                        }
                        prePreIndex = centerIndex;
                    }
                    prePreIndex = originalPrePreIndex;
                    preIndex = originalPreIndex;
                }


            }
        }

        auto attributes = SharedBytes((int64_t)lineAttributes.data(), (int32_t)lineAttributes.size(), (int32_t)sizeof(float));
        auto indices = SharedBytes((int64_t)lineIndices.data(), (int32_t)lineIndices.size(), (int32_t)sizeof(uint32_t));
        line->setLines(attributes, indices, origin, is3d);
    }

    static void pushLineVertex(const Vec3D &p, const Vec3D &extrude, const float extrudeScale, const float side,
                               const float prefixTotalLineLength, const float prefixCorrection, const int lineStyleIndex, const bool addTriangle,
                               const bool reverse, uint32_t &vertexCount, int32_t &prePreIndex, int32_t &preIndex,
                               std::vector<float> &lineAttributes, std::vector<uint32_t> &lineIndices, bool is3d) {

        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);
        if (is3d) {
            lineAttributes.push_back(p.z);
        }
        lineAttributes.push_back(extrude.x * extrudeScale);
        lineAttributes.push_back(extrude.y * extrudeScale);
        if (is3d) {
            lineAttributes.push_back(extrude.z * extrudeScale);
        }

        // Line Side
        lineAttributes.push_back((float)side);

        // Segment Start Length Position (length prefix sum)
        lineAttributes.push_back(prefixTotalLineLength);
        lineAttributes.push_back(prefixCorrection);

        // Style Info
        lineAttributes.push_back(lineStyleIndex);

        uint32_t newIndex = vertexCount++;

        if (addTriangle) {
            if (prePreIndex != -1 && preIndex != -1) {
                if (reverse) {
                    lineIndices.push_back(newIndex);
                    lineIndices.push_back(preIndex);
                    lineIndices.push_back(prePreIndex);
                } else {
                    lineIndices.push_back(prePreIndex);
                    lineIndices.push_back(preIndex);
                    lineIndices.push_back(newIndex);
                }
            }
        }
        prePreIndex = preIndex;
        preIndex = newIndex;
    }

  private:
    static void reserveEstimatedNumVertices(const std::vector<std::tuple<std::vector<Vec3D>, int>> &lines,
                                            LineJoinType joinType, LineCapType capType, bool is3d,
                                            std::vector<float> &lineAttributes, std::vector<uint32_t> &lineIndices)
    {
        // Empirical values: simple linear fit for observed number of vertices and triangles,
        // biased to lightly overestimate -- slightly too large reservation is less wasted memory
        // than slightly understimating and then doubling it.
        // JoinType does not appear to make a large enough difference to warrant using different parameters here.
        const int64_t numVertexPointFactor = 5; // fitted value was ~4.999, integer seems close enough
        const int64_t numVertexOffset = -3;
        const int64_t numTrianglePointFactor = 4; // fitted value was ~4.03, integer seems close enough
        const int64_t numTriangleOffset = -5;

        const int64_t capVertices = capType == LineCapType::ROUND ? 24 : 0;
        const int64_t capTriangles = capType == LineCapType::ROUND ? 24 : 0;

        size_t numVertices = 0;
        size_t numTriangles = 0;
        for(auto &[lineCoords, lineStyleIndex]  : lines) {
            int64_t numPoints = (int64_t)lineCoords.size();
            if(numPoints < 2) {
                continue;
            }
            numVertices += numPoints*numVertexPointFactor + numVertexOffset + capVertices;
            numTriangles += numPoints*numTrianglePointFactor + numTriangleOffset + capTriangles;
        }

        size_t numAttributesPerVertex = is3d ? 9 : 7;
        lineAttributes.reserve(numVertices * numAttributesPerVertex);
        lineIndices.reserve(numTriangles * 3);
    }
};
