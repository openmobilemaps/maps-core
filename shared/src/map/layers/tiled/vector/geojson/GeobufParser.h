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

#include "DataRef.hpp"
#include "GeoJsonTypes.h"
#include "StringInterner.h"

class GeobufParser {
  public:
    /**
     * Decode Geobuf payload into GeoJSON model.
     *
     * @throws std::runtime_error on malformed Geobuf payload
     * @throws nlohmann::json::exception on malformed decoded GeoJSON
     * @returns not-null
     */
    static std::shared_ptr<GeoJson> getGeoJson(const ::djinni::DataRef &geobuf, StringInterner &stringTable);
};
