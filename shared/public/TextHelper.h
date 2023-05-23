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

#include "FontData.h"
#include "MapInterface.h"
#include "TextInfoInterface.h"
#include "TextLayerObject.h"
#include "Vec2F.h"
#include "Quad2dD.h"
#include "SymbolAlignment.h"

#include <optional>

struct BreakResult {
    BreakResult(int index, bool keepLetter) : index(index), keepLetter(keepLetter) {};
    int index;
    bool keepLetter;
};

class TextHelper {
  public:
    TextHelper() {};
    TextHelper(const std::shared_ptr<MapInterface> &mapInterface);

    void setMapInterface(const std::weak_ptr< ::MapInterface> &mapInterface);

    virtual std::shared_ptr<TextLayerObject> textLayerObject(const std::shared_ptr<TextInfoInterface> &text,
                                                             std::optional<FontData> fontData,
                                                             Vec2F offset,
                                                             double lineHeight,
                                                             double letterSpacing,
                                                             int64_t maxCharacterWidth,
                                                             double maxCharacterAngle,
                                                             SymbolAlignment rotationAlignment);

    static std::string uppercase(const std::string &string);

    static Quad2dD rotateQuad2d(const Quad2dD &quad, const Vec2D &aroundPoint, double angleDegrees);

    static std::vector<std::string> splitWstring(const std::string &word);

    static std::vector<BreakResult> bestBreakIndices(std::vector<std::string> &letters, int64_t maxCharacterWidth);

  private:
    std::weak_ptr<MapInterface> mapInterface;
};
