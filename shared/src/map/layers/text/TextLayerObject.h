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

#include "TextShaderInterface.h"
#include "Coord.h"
#include "LineStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Line2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"
#include "TextInterface.h"

class TextLayerObject : public LayerObjectInterface {
  public:
    TextLayerObject(const std::shared_ptr<TextInterface> &text, const std::shared_ptr<TextShaderInterface> &shader, const Coord& referencePoint, float referenceSize);

    virtual ~TextLayerObject() {};

    virtual void update() {};

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig();

    virtual std::shared_ptr<TextInterface> getTextObject() { return text; }
    virtual std::shared_ptr<TextShaderInterface> getShader() { return shader; }

    virtual float getReferenceSize() { return referenceSize; }
    virtual Coord getReferencePoint() { return referencePoint; }

  private:
    std::shared_ptr<TextInterface> text;
    std::shared_ptr<TextShaderInterface> shader;
    std::vector<std::shared_ptr<RenderConfigInterface>> renderConfig;
    Coord referencePoint;

    float referenceSize;
};
