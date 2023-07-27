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

#include "Vec2F.h"
#include "SymbolVectorLayerDescription.h"
#include "Value.h"
#include "SymbolInfo.h"
#include "StretchShaderInterface.h"
#include "OBB2D.h"
#include "MapInterface.h"
#include "Tiled2dMapVectorFontProvider.h"
#include "CoordinateConversionHelperInterface.h"
#include "Actor.h"
#include "BoundingBox.h"
#include "SpriteData.h"

class Tiled2dMapVectorSymbolLabelObject {
public:
    Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                      const std::shared_ptr<FeatureContext> featureContext,
                                      const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                      const std::vector<FormattedStringEntry> &text,
                                      const std::string &fullText,
                                      const ::Coord &coordinate,
                                      const std::optional<std::vector<Coord>> &lineCoordinates,
                                      const Anchor &textAnchor,
                                      const std::optional<double> &angle,
                                      const TextJustify &textJustify,
                                      const std::shared_ptr<FontLoaderResult> fontResult,
                                      const Vec2F &offset,
                                      const double lineHeight,
                                      const double letterSpacing,
                                      const int64_t maxCharacterWidth,
                                      const double maxCharacterAngle,
                                      const SymbolAlignment rotationAlignment,
                                      const TextSymbolPlacement &textSymbolPlacement);

    int getCharacterCount();

    void setupProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier);

    void updateProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation, const float alpha);

    std::shared_ptr<FontLoaderResult> getFont() {
        return fontResult;
    }

    Quad2dD boundingBox;

    bool isOpaque = true;

    Vec2D dimensions = Vec2D(0.0, 0.0);
private:

    void updatePropertiesPoint(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation);
    double updatePropertiesLine(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation);

    std::pair<int, double> findReferencePointIndices();
    Vec2D pointAtIndex(const std::pair<int, double> &index, bool useRender);
    std::pair<int, double> indexAtDistance(const std::pair<int, double> &index, double distance);

    const std::shared_ptr<SymbolVectorLayerDescription> description;
    const std::shared_ptr<FeatureContext> featureContext;
    const TextSymbolPlacement textSymbolPlacement;
    const SymbolAlignment rotationAlignment;
    TextJustify textJustify;
    const Anchor textAnchor;
    const Vec2F offset;

    float spaceAdvance = 0.0f;

    const double lineHeight;
    const double letterSpacing;
    const double maxCharacterAngle;

    const std::shared_ptr<FontLoaderResult> fontResult;

    Coord referencePoint = Coord(0,0,0,0);
    float referenceSize;


    std::vector<Vec2D> centerPositions;

    struct SplitInfo {
        SplitInfo(int g, float s) : glyphIndex(g), scale(s) {};
        int glyphIndex;
        float scale;
    };

    int characterCount = 0;
    std::vector<SplitInfo> splittedTextInfo;

    const std::string fullText;

    size_t renderLineCoordinatesCount;
    std::vector<Coord> renderLineCoordinates;
    std::optional<std::vector<Coord>> lineCoordinates;

    double textSize = 0;
    double textRotate = 0;
    double textPadding = 0;
    SymbolAlignment textAlignment = SymbolAlignment::AUTO;
};
