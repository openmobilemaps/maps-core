/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "CoordinateConversionHelperInterface.h"
#include "RectCoord.h"
#include "StringInterner.h"
#include "Tiled2dMapTileInfo.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorSettings.h"
#include "VectorTileSourceFormat.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>

class VectorTileDataParser {
public:
    static Tiled2dMapVectorTileInfo::FeatureMap parse(const uint8_t *data,
                                                      size_t dataLength,
                                                      Tiled2dMapTileInfo tileInfo,
                                                      const RectCoord &tileBounds,
                                                      const std::optional<Tiled2dMapVectorSettings> &vectorSettings,
                                                      const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                      StringInterner &stringTable,
                                                      const std::unordered_set<std::string> &layersToDecode,
                                                      VectorTileSourceFormat format,
                                                      const std::function<bool()> &shouldContinue);
};

