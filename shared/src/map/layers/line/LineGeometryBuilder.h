//
//  LineGeometryBuilder.h
//  MapCore
//
//  Created by Nicolas Märki on 26.05.2025.
//

#include "Vec3DHelper.h"

class LineGeometryBuilder {
  public:
    static void buildLines(const std::shared_ptr<LineGroup2dInterface> &line, const std::vector<std::tuple<std::vector<Vec3D>, int>> &lines, const Vec3D &origin, LineCapType capType,
                    LineJoinType joinType, bool is3d) {
        std::vector<uint32_t> lineIndices;
        std::vector<float> lineAttributes;
        uint32_t vertexCount = 0;

        int numLines = (int)lines.size();

        for (int lineIndex = numLines - 1; lineIndex >= 0; lineIndex--) {
            auto const &[renderCoords, lineStyleIndex] = lines[lineIndex];

            int pointCount = (int)renderCoords.size();

            float prefixTotalLineLength = 0.0;
            float lineLength = 0;
            float cosAngle = 0, cosHalfAngle = 0;

            Vec3D pLast(0, 0, 0), normal(0, 0, 0), lastNormal(0, 0, 0), lineVec(0, 0, 0), lastLineVec(0, 0, 0),
                pOriginLine(0, 0, 0);

            int32_t preIndex = -1, prePreIndex = -1;

            for (int i = 0; i < pointCount; i++) {
                const Vec3D &p = renderCoords[i];

                prefixTotalLineLength += lineLength;

                float extrudeScale = 1.0;
                LineJoinType vertexJoinType = joinType;

                Vec3D extrude(0, 0, 0), extrudeLineVec(0, 0, 0);

                if (i > 0) {
                    lastNormal = normal;
                    lastLineVec = lineVec;
                }

                if (i < pointCount - 1) {
                    const Vec3D &pNext = renderCoords[i + 1];

                    lineVec = pNext - p;
                    lineLength = Vec3DHelper::length(lineVec);
                    if (lineLength == 0) {
                        continue;
                    }
                    lineVec /= lineLength;

                    if (is3d) {
                        pOriginLine = Vec3DHelper::normalize(Vec3D(p.x + origin.x, p.y + origin.y, p.z + origin.z));
                        normal = Vec3DHelper::normalize(Vec3DHelper::crossProduct(pOriginLine, lineVec));
                    } else {
                        normal = Vec3DHelper::normalize(Vec3D(-lineVec.y, lineVec.x, 0.0));
                    }

                    if (i > 0) {
                        extrude = (normal + lastNormal);
                        double extrudeLength = Vec3DHelper::length(extrude);
                        if (extrudeLength > 0) {
                            extrude /= extrudeLength;
                        }

                        if (is3d) {
                            cosAngle = Vec3DHelper::dotProduct(normal, lastNormal);
                            cosHalfAngle = Vec3DHelper::dotProduct(extrude, normal);
                        } else {
                            cosAngle = normal.x * lastNormal.x + normal.y * lastNormal.y;
                            cosHalfAngle = extrude.x * normal.x + extrude.y * normal.y;
                        }

                        extrudeScale = cosHalfAngle != 0 ? std::abs(1.0 / cosHalfAngle) : 1.0;
                        if (extrudeScale > 2.0) {
                            vertexJoinType = LineJoinType::BEVEL;
                            extrudeScale = 2.0;
                        }
                    } else {
                        extrude = normal;
                        extrudeLineVec = lineVec;
                    }
                } else if (i > 0) {
                    lineLength = 0;
                    extrude = lastNormal;
                    extrudeLineVec = -lastLineVec;
                } else {
                    lineLength = 0;
                    continue;
                }

                float endSide = 0;
                if (i == 0) {
                    endSide = -1;
                } else if (i == pointCount - 1) {
                    endSide = 1;
                }

                if (endSide != 0 && capType == LineCapType::ROUND) {
                    auto originalPrePreIndex = prePreIndex;
                    auto originalPreIndex = preIndex;
                    pushLineVertex(p, Vec3D(0, 0, 0), 1.0, 0, prefixTotalLineLength, lineStyleIndex, true, false, vertexCount,
                                   prePreIndex, preIndex, lineAttributes, lineIndices,
                                   is3d);
                    int32_t centerIndex = preIndex, firstIndex = -1, lastIndex = -1;
                    for (float r = -1; r <= 1; r += 0.2) {
                        Vec3D roundExtrude = Vec3DHelper::normalize(extrude * r - extrudeLineVec * (1.0 - abs(r)));
                        pushLineVertex(p, roundExtrude, 1.0, r, prefixTotalLineLength, lineStyleIndex, true, endSide == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices,
                                       is3d);
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
                    if (side * cosAngle < 0 && endSide == 0 && extrudeScale > 1.1 && joinType != LineJoinType::MITER) {
                        const double approxAngle = 2 * std::sqrt(2 - 2 * cosHalfAngle);
                        // 2.86 ~= 180/pi / 20 -> approximately one slice per 20 degrees
                        double stepSize = (joinType == LineJoinType::ROUND) ? 1.0 / round(approxAngle * 2.86) : 1.0;
                        for (float r = 0; r <= 1; r += stepSize) {
                            pointExtrude = Vec3DHelper::normalize(lastNormal * (1.0 - r) + normal * r) * (double)side;
                            pushLineVertex(p, pointExtrude, 1.0, side, prefixTotalLineLength, lineStyleIndex, true, side == -1,
                                           vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices,
                                           is3d);
                            std::swap(prePreIndex, preIndex);
                        }
                        std::swap(prePreIndex, preIndex);
                    } else {
                        pushLineVertex(p, pointExtrude, extrudeScale, side, prefixTotalLineLength, lineStyleIndex, true, side == -1,
                                       vertexCount, prePreIndex, preIndex, lineAttributes, lineIndices,
                                           is3d);
                    }
                }
            }
        }

        auto attributes = SharedBytes((int64_t)lineAttributes.data(), (int32_t)lineAttributes.size(), (int32_t)sizeof(float));
        auto indices = SharedBytes((int64_t)lineIndices.data(), (int32_t)lineIndices.size(), (int32_t)sizeof(uint32_t));
        line->setLines(attributes, indices, origin, is3d);
    }

    static void pushLineVertex(const Vec3D &p, const Vec3D &extrude, const float extrudeScale, const float side,
                        const float prefixTotalLineLength, const int lineStyleIndex, const bool addTriangle, const bool reverse,
                        uint32_t &vertexCount, int32_t &prePreIndex, int32_t &preIndex, std::vector<float> &lineAttributes,
                        std::vector<uint32_t> &lineIndices, bool is3d) {
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
};
