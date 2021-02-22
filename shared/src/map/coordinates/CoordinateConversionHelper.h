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
#include "CoordinateConversionHelperInterface.h"
#include "HashedTuple.h"
#include "MapCoordinateSystem.h"
#include "QuadCoord.h"
#include "RectCoord.h"
#include "string"
#include <mutex>
#include <unordered_map>
#include <vector>

class CoordinateConversionHelper : public CoordinateConversionHelperInterface {
  public:
    CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem);

    virtual void registerConverter(const std::shared_ptr<CoordinateConverterInterface> &converter) override;

    virtual Coord convert(const std::string &to, const Coord &coordinate) override;

    virtual RectCoord convertRect(const std::string &to, const RectCoord &rect) override;

    virtual RectCoord convertRectToRenderSystem(const RectCoord &rect) override;

    virtual QuadCoord convertQuad(const std::string &to, const QuadCoord &quad) override;

    virtual QuadCoord convertQuadToRenderSystem(const QuadCoord &quad) override;

    virtual Coord convertToRenderSystem(const Coord &coordinate) override;

  private:
    std::unordered_map<std::tuple<std::string, std::string>, std::shared_ptr<CoordinateConverterInterface>> fromToConverterMap;

    std::unordered_map<std::tuple<std::string, std::string>, std::vector<std::shared_ptr<CoordinateConverterInterface>>>
        converterHelper;

    std::string mapCoordinateSystemIdentier;

    std::recursive_mutex converterMutex;

    void precomputeConverterHelper();
};
