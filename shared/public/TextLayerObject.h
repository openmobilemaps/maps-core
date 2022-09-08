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
#include "LayerObjectInterface.h"
#include "Line2dInterface.h"
#include "LineStyle.h"
#include "RenderConfig.h"
#include "TextInterface.h"
#include "TextShaderInterface.h"
#include "Vec2D.h"
#include "RectCoord.h"
#include "Quad2dInterface.h"
#include "AlphaShaderInterface.h"
#include "Anchor.h"
#include "Vec2F.h"
#include "RectD.h"

class TextLayerObject : public LayerObjectInterface {
  public:
    TextLayerObject(const std::shared_ptr<TextInterface> &text,
                    const std::shared_ptr<TextShaderInterface> &shader,
                    const Coord &referencePoint, float referenceSize,
                    RectCoord boundingBox);

    virtual ~TextLayerObject(){};

    virtual void update(){};

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig();

    virtual std::shared_ptr<TextInterface> getTextObject() { return text; }
    virtual std::shared_ptr<TextShaderInterface> getShader() { return shader; }

    virtual float getReferenceSize() { return referenceSize; }
    virtual Coord getReferencePoint() { return referencePoint; }

    RectCoord boundingBox;

    std::optional<std::string> getCurrentSymbolName() { return symbolName; }
    void setCurrentSymbolName(std::optional<std::string> symbolName) { this->symbolName = symbolName; }

  private:
    std::shared_ptr<TextInterface> text;
    std::shared_ptr<TextShaderInterface> shader;

    Vec2D currentSize = Vec2D(1.0, 1.0);
    RectD currentUv = RectD(0.0, 0.0, 1.0, 1.0);
    Anchor currentIconAnchor = Anchor::CENTER;
    Vec2F currentIconOffset = Vec2F(0.0, 0.0);
    
    std::vector<std::shared_ptr<RenderConfigInterface>> renderConfig;

    Coord referencePoint;
    float referenceSize;

    std::optional<std::string> symbolName;
};
