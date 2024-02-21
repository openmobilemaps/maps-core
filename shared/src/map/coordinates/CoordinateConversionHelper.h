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
    CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem, bool enforceLtrTtb);

    /**
     * This instance is independent of the map and does not know about the rendering system.
     * It can not be used to convert coordinates into rendering space.
     */
    CoordinateConversionHelper();

    virtual void registerConverter(const std::shared_ptr<CoordinateConverterInterface> &converter) override;

    virtual Coord convert(const int32_t to, const Coord &coordinate) override;

    virtual RectCoord convertRect(const int32_t to, const RectCoord &rect) override;

    virtual RectCoord convertRectToRenderSystem(const RectCoord &rect) override;

    virtual QuadCoord convertQuad(const int32_t to, const QuadCoord &quad) override;

    virtual QuadCoord convertQuadToRenderSystem(const QuadCoord &quad) override;

    virtual Coord convertToRenderSystem(const Coord &coordinate) override;

  private:
    void addDefaultConverters();

    std::unordered_map<std::tuple<int32_t, int32_t>, std::shared_ptr<CoordinateConverterInterface>> fromToConverterMap;

    std::unordered_map<std::tuple<int32_t, int32_t>, std::vector<std::shared_ptr<CoordinateConverterInterface>>>
        converterHelper;

    std::shared_ptr<CoordinateConverterInterface> renderSystemConverter;

    int32_t mapCoordinateSystemIdentifier;
    bool enforceLtrTtb = false;

    std::recursive_mutex converterMutex;

    void precomputeConverterHelper();
};
