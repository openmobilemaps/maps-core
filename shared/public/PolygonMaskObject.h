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

#include "GraphicsObjectFactoryInterface.h"
#include "CoordinateConversionHelperInterface.h"
#include "GraphicsObjectInterface.h"
#include "PolygonMaskObjectInterface.h"
#include "Polygon2dInterface.h"
#include "Coord.h"
#include "PolygonCoord.h"

class PolygonMaskObject: public PolygonMaskObjectInterface {
public:
    PolygonMaskObject(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                      const std::shared_ptr<::CoordinateConversionHelperInterface> &conversionHelper,
                      bool is3D);

    virtual void setPolygons(const std::vector<::PolygonCoord> & polygons,
                             const Vec3D & origin, std::optional<float> subdivisionFactor = std::nullopt);

    virtual void setPolygon(const ::PolygonCoord & polygon,
                            const Vec3D & origin, std::optional<float> subdivisionFactor = std::nullopt);

    virtual void setPolygons(const std::vector<::PolygonCoord> & polygons,
                             const Vec3D & origin) override {
        setPolygons(polygons, origin, std::nullopt);
    };

    virtual void setPolygon(const ::PolygonCoord & polygon,
                            const Vec3D & origin) override {
        setPolygon(polygon, origin, std::nullopt);
    }

    void setPositions(const std::vector<Coord> &positions, const ::Vec3D & origin, const std::vector<std::vector<Coord>> &holes);

    virtual std::shared_ptr<::Polygon2dInterface> getPolygonObject() override;

private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Polygon2dInterface> polygon;
    bool is3D;
};

