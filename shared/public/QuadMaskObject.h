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
#include "Quad2dInterface.h"
#include "RectCoord.h"
#include "QuadCoord.h"

class QuadMaskObject {
public:
    QuadMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);

    QuadMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
            const ::RectCoord &rectCoord);

    void setRectCoord(const ::RectCoord &rectCoord);

    void setPosition(const ::Coord &coord, double width, double height);

    void setPositions(const ::QuadCoord &coords);

    std::shared_ptr<Quad2dInterface> getQuadObject();

    std::shared_ptr<MaskingObjectInterface> getMaskObject();

private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Quad2dInterface> quad;

};
