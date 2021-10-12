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

#include "MapInterface.h"
#include "TextLayerObject.h"
#include "TextInfoInterface.h"
#include "FontData.h"
#include "Vec2F.h"

#include <optional>

class TextHelper {
  public:
    TextHelper(const std::shared_ptr<MapInterface> &mapInterface);

    virtual std::shared_ptr<TextLayerObject> textLayer(const std::shared_ptr<TextInfoInterface> &text, std::optional<FontData> fontData, Vec2F offset);

    static std::string uppercase(const std::string& string);

  private:
    std::shared_ptr<MapInterface> mapInterface;
};
