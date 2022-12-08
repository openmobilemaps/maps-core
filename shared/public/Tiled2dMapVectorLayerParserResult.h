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

#include "VectorMapDescription.h"
#include "LoaderStatus.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>

struct Tiled2dMapVectorLayerParserResult {
    std::shared_ptr<VectorMapDescription> mapDescription;
    LoaderStatus status;
    std::optional<std::string> errorCode;

    Tiled2dMapVectorLayerParserResult(std::shared_ptr<VectorMapDescription> mapDescription_, LoaderStatus status_,
                                      std::optional<std::string> errorCode_)
            : mapDescription(std::move(mapDescription_)),
              status(std::move(status_)),
              errorCode(std::move(errorCode_)) {}

};