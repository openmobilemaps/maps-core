#pragma once

template <uint8_t I>
class clipper {
public:
    clipper(double k1_, double k2_) : k1(k1_), k2(k2_) {}

    const double k1;
    const double k2;

    std::shared_ptr<GeoJsonGeometry> clip(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        std::shared_ptr<GeoJsonGeometry> result;
        switch (geometry->featureContext->geomType) {
            case vtzero::GeomType::POINT: {
                result = clipPoints(geometry);
                break;
            }
            case vtzero::GeomType::LINESTRING: {
                result = clipLines(geometry);
                break;
            }
            case vtzero::GeomType::POLYGON: {
                result = clipPolygons(geometry);
                break;
            }
            case vtzero::GeomType::UNKNOWN: {
                result = geometry;
                break;
            }
        }
        updateMinMax(result);
        return result;
    }

private:

    void updateMinMax(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        for (const auto& points : geometry->coordinates) {
            for (const auto& point : points) {
                geometry->bboxMin.x = std::min(point.x, geometry->bboxMin.x);
                geometry->bboxMin.y = std::min(point.y, geometry->bboxMin.y);
                geometry->bboxMax.x = std::max(point.x, geometry->bboxMax.x);
                geometry->bboxMax.y = std::max(point.y, geometry->bboxMax.y);
            }
        }

        for (const auto& hole : geometry->holes) {
            for (const auto& points : hole) {
                for (const auto& point : points) {
                    geometry->bboxMin.x = std::min(point.x, geometry->bboxMin.x);
                    geometry->bboxMin.y = std::min(point.y, geometry->bboxMin.y);
                    geometry->bboxMax.x = std::max(point.x, geometry->bboxMax.x);
                    geometry->bboxMax.y = std::max(point.y, geometry->bboxMax.y);
                }
            }
        }
    }

    std::shared_ptr<GeoJsonGeometry> clipPoints(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        std::shared_ptr<FeatureContext> featureContext = geometry->featureContext;
        std::vector<std::vector<::Coord>> coordinates;
        std::vector<std::vector<std::vector<::Coord>>> holes;

        std::vector<Coord> temp;
        for (const auto& points : geometry->coordinates) {
            for (const auto& point : points) {
                const double ak = get<I>(point);
                if (ak >= k1 && ak <= k2)
                    temp.emplace_back(point);
            }
        }
        coordinates.push_back(temp);
        return std::make_shared<GeoJsonGeometry>(featureContext, coordinates, holes);
    }

    std::shared_ptr<GeoJsonGeometry> clipLines(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        std::shared_ptr<FeatureContext> featureContext = geometry->featureContext;
        std::vector<std::vector<::Coord>> coordinates;
        std::vector<std::vector<std::vector<::Coord>>> holes;

        std::vector<std::vector<Coord>> slices;

        for (const auto& line : geometry->coordinates) {
            clipLine(line, slices);
        }

        coordinates = slices;

        return std::make_shared<GeoJsonGeometry>(featureContext, coordinates, holes);
    }

    std::shared_ptr<GeoJsonGeometry> clipPolygons(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        std::shared_ptr<FeatureContext> featureContext = geometry->featureContext;
        std::vector<std::vector<::Coord>> coordinates;
        std::vector<std::vector<std::vector<::Coord>>> holes;

        for (int i = 0; i < geometry->coordinates.size(); i++) {
            const auto &clippedRing = clipRing(geometry->coordinates[i]);
            if (!clippedRing.empty()) {
                coordinates.push_back(clippedRing);

                const auto &hole = geometry->holes[i];
                std::vector<std::vector<::Coord>> tmpHole;
                for (const auto& polygon : hole) {
                    const auto &clippedRing = clipRing(polygon);
                    if (!clippedRing.empty()) {
                        tmpHole.push_back(clippedRing);
                    }
                }
                holes.push_back(tmpHole);
            }
        }

        return std::make_shared<GeoJsonGeometry>(featureContext, coordinates, holes);
    }

    void clipLine(const std::vector<Coord>& line,std::vector<std::vector<Coord>> &slices) const {
        const size_t len = line.size();
        double segLen = 0.0;
        double t = 0.0;

        if (len < 2)
            return;

        std::vector<Coord> slice;

        for (size_t i = 0; i < (len - 1); ++i) {
            const auto& a = line[i];
            const auto& b = line[i + 1];
            const double ak = get<I>(a);
            const double bk = get<I>(b);
            const bool isLastSeg = (i == (len - 2));


            if (ak < k1) {
                if (bk > k2) { // ---|-----|-->
                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));

                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));

                    slices.emplace_back(std::move(slice));

                    slice = std::vector<Coord>{};

                } else if (bk > k1) { // ---|-->  |
                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));

                    if (isLastSeg) slice.emplace_back(b); // last point

                } else if (bk == k1 && !isLastSeg) { // --->|..  |

                    slice.emplace_back(b);
                }
            } else if (ak > k2) {
                if (bk < k1) { // <--|-----|---
                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));

                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));

                    slices.emplace_back(std::move(slice));

                    slice = std::vector<Coord>{};

                } else if (bk < k2) { // |  <--|---
                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));

                    if (isLastSeg) slice.emplace_back(b); // last point

                } else if (bk == k2 && !isLastSeg) { // |  ..|<---

                    slice.emplace_back(b);
                }
            } else {
                slice.emplace_back(a);

                if (bk < k1) { // <--|---  |
                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));

                    slices.emplace_back(std::move(slice));
                    slice = std::vector<Coord>{};

                } else if (bk > k2) { // |  ---|-->
                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));

                    slices.emplace_back(std::move(slice));
                    slice = std::vector<Coord>{};

                } else if (isLastSeg) { // | --> |
                    slice.emplace_back(b);
                }
            }
        }

        if (!slice.empty()) { // add the final slice
            slices.emplace_back(std::move(slice));
        }
    }

    std::vector<Coord> clipRing(const std::vector<Coord>& ring) const {
        const size_t len = ring.size();
        std::vector<Coord> slice;

        if (len < 2)
            return slice;

        for (size_t i = 0; i < (len - 1); ++i) {
            const auto& a = ring[i];
            const auto& b = ring[i + 1];
            const double ak = get<I>(a);
            const double bk = get<I>(b);

            if (ak < k1) {
                if (bk > k1) {
                    // ---|-->  |
                    slice.emplace_back(intersect<I>(a, b, k1, calc_progress<I>(a, b, k1)));
                    if (bk > k2)
                        // ---|-----|-->
                        slice.emplace_back(intersect<I>(a, b, k2, calc_progress<I>(a, b, k2)));
                    else if (i == len - 2)
                        slice.emplace_back(b); // last point
                }
            } else if (ak > k2) {
                if (bk < k2) { // |  <--|---
                    slice.emplace_back(intersect<I>(a, b, k2, calc_progress<I>(a, b, k2)));
                    if (bk < k1) // <--|-----|---
                        slice.emplace_back(intersect<I>(a, b, k1, calc_progress<I>(a, b, k1)));
                    else if (i == len - 2)
                        slice.emplace_back(b); // last point
                }
            } else {
                // | --> |
                slice.emplace_back(a);
                if (bk < k1)
                    // <--|---  |
                    slice.emplace_back(intersect<I>(a, b, k1, calc_progress<I>(a, b, k1)));
                else if (bk > k2)
                    // |  ---|-->
                    slice.emplace_back(intersect<I>(a, b, k2, calc_progress<I>(a, b, k2)));
            }
        }

        // close the polygon if its endpoints are not the same after clipping
        if (!slice.empty()) {
            const auto& first = slice.front();
            const auto& last = slice.back();
            if (first != last) {
                slice.emplace_back(first);
            }
        }

        return slice;
    }
};

/* clip features between two axis-parallel lines:
 *     |        |
 *  ___|___     |     /
 * /   |   \____|____/
 *     |        |
 */

template <uint8_t I>
inline std::vector<std::shared_ptr<GeoJsonGeometry>> clip(const std::vector<std::shared_ptr<GeoJsonGeometry>> &features,
                        const double k1,
                        const double k2,
                        const double minAll,
                        const double maxAll) {

    if (minAll >= k1 && maxAll < k2) // trivial accept
        return features;

    if (maxAll < k1 || minAll >= k2) // trivial reject
        return {};

    std::vector<std::shared_ptr<GeoJsonGeometry>> clipped;
    clipped.reserve(features.size());

    clipper<I> clip(k1, k2);

    for (const auto& feature : features) {
        const double min = get<I>(feature->bboxMin);
        const double max = get<I>(feature->bboxMax);

        if (min >= k1 && max < k2) { // trivial accept
            clipped.emplace_back(feature);
        } else if (max < k1 || min >= k2) { // trivial reject
            continue;
        } else {
            const auto clippedFeature = clip.clip(feature);
            if (!clippedFeature->coordinates.empty()) {
                clipped.emplace_back(clippedFeature);
            }
        }
    }

    return clipped;
}
