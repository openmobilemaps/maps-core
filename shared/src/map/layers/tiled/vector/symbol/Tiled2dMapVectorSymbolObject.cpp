/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolObject.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"

Tiled2dMapVectorSymbolObject::Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                                           const Actor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                           const Tiled2dMapTileInfo &tileInfo,
                                                           const std::string &layerIdentifier,
                                                           const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                           const std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>> &symbolInfo) {
    auto strongMapInterface = mapInterface.lock();
    auto objectFactory = strongMapInterface ? strongMapInterface->getGraphicsObjectFactory() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!objectFactory || !camera) {
        return;
    }

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    const auto &[context, symbol] = symbolInfo;

    auto fontResult = loadFont(symbol->getFont());
    if (fontResult.status != LoaderStatus::OK) {
        return nullptr;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, context);

    auto textOffset = description->style.getTextOffset(evalContext);

    const auto textRadialOffset = description->style.getTextRadialOffset(evalContext);
    // TODO: currently only shifting to top right
    textOffset.x += textRadialOffset;
    textOffset.y -= textRadialOffset;

    const auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

    const auto textObject = textHelper.textLayerObject(symbol,
                                                       fontResult.fontData,
                                                       textOffset,
                                                       description->style.getTextLineHeight(evalContext),
                                                       letterSpacing,
                                                       description->style.getTextMaxWidth(evalContext),
                                                       description->style.getTextMaxAngle(evalContext),
                                                       description->style.getTextRotationAlignment(evalContext));

    if (textObject) {
        int64_t const symbolSortKey = description->style.getSymbolSortKey(evalContext);
        std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> wrapper = std::make_shared<Tiled2dMapVectorSymbolFeatureWrapper>(
                context, symbol, textObject, symbolSortKey);

#ifdef DRAW_TEXT_BOUNDING_BOXES
        wrapper->boundingBoxShader = mapInterface->getShaderFactory()->createColorShader();
        wrapper->boundingBoxShader->setColor(0.0, 1.0, 0.0, 0.5);
        std::shared_ptr<Quad2dInterface> quadObject = mapInterface->getGraphicsObjectFactory()->createQuad(wrapper->boundingBoxShader->asShaderProgramInterface());
        quadObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

        wrapper->boundingBox = quadObject;
#endif

#ifdef DRAW_TEXT_LINES
        if(text->getLineCoordinates()) {
            auto list = ColorStateList(Color(0.0, 1.0, 0.0, 1.0), Color(0.0, 1.0, 0.0, 1.0));

            std::shared_ptr<LineInfoInterface> lineInfo = LineFactory::createLine("draw_text_lines", *text->getLineCoordinates(), LineStyle(list, list, 1.0, 1.0, SizeType::SCREEN_PIXEL, 3.0, {}, LineCapType::ROUND));

            auto objectFactory = mapInterface->getGraphicsObjectFactory();
            auto shaderFactory = mapInterface->getShaderFactory();

            auto shader = shaderFactory->createColorLineShader();
            auto lineGraphicsObject = objectFactory->createLine(shader->asShaderProgramInterface());

            auto lineObject = std::make_shared<Line2dLayerObject>(mapInterface->getCoordinateConverterHelper(), lineGraphicsObject, shader);

            lineObject->setStyle(lineInfo->getStyle());
            lineObject->setPositions(lineInfo->getCoordinates());
            lineObject->getLineObject()->setup(mapInterface->getRenderingContext());

            wrapper->lineObject = lineObject;
        }
#endif

        isInteractable = description->isInteractable(evalContext);
    }
}

void Tiled2dMapVectorSymbolObject::setCollisionAt(float zoom, bool isCollision) {
    collisionMap[zoom] = isCollision;
}

bool Tiled2dMapVectorSymbolObject::hasCollision(float zoom) {
    {
        // TODO: make better.
        float minDiff = std::numeric_limits<float>::max();
        float min = std::numeric_limits<float>::max();

        bool collides = true;

        for(auto& cm : collisionMap) {
            float d = abs(zoom - cm.first);

            if(d < minDiff && d < min) {
                min = d;
                collides = cm.second;
            }
        }

        return collides;
    }

}