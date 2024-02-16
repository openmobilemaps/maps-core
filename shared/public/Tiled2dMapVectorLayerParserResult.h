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

#include "VectorMapSourceDescription.h"
#include "LoaderStatus.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>

struct Tiled2dMapVectorLayerParserResult {
    std::shared_ptr<VectorMapDescription> mapDescription;
    LoaderStatus status;
    std::optional<std::string> errorCode;
    std::optional<std::string> metadata;

    Tiled2dMapVectorLayerParserResult(std::shared_ptr<VectorMapDescription> mapDescription,
                                      LoaderStatus status,
                                      std::optional<std::string> errorCode,
                                      std::optional<std::string> metadata)
            : mapDescription(std::move(mapDescription)),
              status(std::move(status)),
              errorCode(std::move(errorCode)),
              metadata(std::move(metadata))
        {}
};
