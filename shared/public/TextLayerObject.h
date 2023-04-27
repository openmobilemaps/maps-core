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

//#define DRAW_TEXT_LETTER_BOXES

#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Line2dInterface.h"
#include "LineStyle.h"
#include "RenderConfig.h"
#include "TextInterface.h"
#include "TextInfoInterface.h"
#include "TextShaderInterface.h"
#include "Vec2D.h"
#include "RectCoord.h"
#include "Quad2dInterface.h"
#include "AlphaShaderInterface.h"
#include "Anchor.h"
#include "Vec2F.h"
#include "RectD.h"
#include "FontData.h"
#include "MapCamera2dInterface.h"
#include "Quad2dD.h"
#include "SymbolAlignment.h"

class TextLayerObject : public LayerObjectInterface {
  public:
    TextLayerObject(const std::shared_ptr<TextInterface> &text,
                    const std::shared_ptr<TextInfoInterface> &textInfo,
                    const std::shared_ptr<TextShaderInterface> &shader,
                    const std::shared_ptr<MapInterface> &mapInterface,
                    const FontData& fontData,
                    const Vec2F &offset,
                    double lineHeight,
                    double letterSpacing,
                    int64_t maxCharacterWidth,
                    double maxCharacterAngle,
                    SymbolAlignment rotationAlignment);

    virtual ~TextLayerObject(){};

    virtual void update() {};

    virtual void layout(float scale);
    virtual void update(float scale);

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig();

    virtual std::shared_ptr<TextInterface> getTextObject() { return text; }
    virtual std::shared_ptr<TextShaderInterface> getShader() { return shader; }

    virtual float getReferenceSize() { return referenceSize; }
    virtual Coord getReferencePoint() { return referencePoint; }

    RectCoord boundingBox;

    std::optional<std::string> getCurrentSymbolName() { return symbolName; }
    void setCurrentSymbolName(std::optional<std::string> symbolName) { this->symbolName = symbolName; }

#ifdef DRAW_TEXT_LETTER_BOXES
    std::vector<Quad2dD> getLetterBoxes();
#endif


    std::vector<float> positions;
    std::vector<float> scales;
    std::vector<float> textureCoordinates;
    std::vector<float> rotations;

  private:
    void update(float scale, bool updateTextObject);
    void layoutPoint(float scale, bool updateTextObject);
    float layoutLine(float scale, bool updateTextObject);

    std::pair<int, double> findReferencePointIndices();
    Coord pointAtIndex(const std::pair<int, double> &index, bool useRender = true);
    std::pair<int, double> indexAtDistance(const std::pair<int, double> &index, double distance);

  private:
    std::shared_ptr<TextInterface> text;
    std::shared_ptr<TextInfoInterface> textInfo;

    std::shared_ptr<TextShaderInterface> shader;

    Vec2D currentSize = Vec2D(1.0, 1.0);
    RectD currentUv = RectD(0.0, 0.0, 1.0, 1.0);
    Anchor currentIconAnchor = Anchor::CENTER;
    Vec2F currentIconOffset = Vec2F(0.0, 0.0);
    
    std::vector<std::shared_ptr<RenderConfigInterface>> renderConfig;

    Coord referencePoint;
    float referenceSize;

    std::optional<std::string> symbolName;
public:
    FontData fontData;
    Vec2F offset;
    double lineHeight;
    double letterSpacing;
    double maxCharacterAngle;
    std::shared_ptr<CoordinateConversionHelperInterface> converter;
    std::shared_ptr<MapCamera2dInterface> camera;
    std::optional<std::vector<::Coord>> lineCoordinates;
    std::vector<::Coord> renderLineCoordinates;

    SymbolAlignment rotationAlignment;

#ifdef DRAW_TEXT_LETTER_BOXES
    std::vector<Quad2dD> letterBoxes;
#endif

    bool rotated = false;

    struct SplitInfo {
        SplitInfo(int g, float s) : glyphIndex(g), scale(s) {};
        int glyphIndex;
        float scale;
    };

    std::vector<SplitInfo> splittedTextInfo;

    std::optional<float> lastScale;
    std::vector<float> vertices;
    std::vector<int16_t> indices;
};
