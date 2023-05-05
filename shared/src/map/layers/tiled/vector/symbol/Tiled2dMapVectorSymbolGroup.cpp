/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolGroup.h"
#include "TextHelper.h"
#include "Vec2DHelper.h"

Tiled2dMapVectorSymbolGroup::Tiled2dMapVectorSymbolGroup(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Actor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                         const Tiled2dMapTileInfo &tileInfo,
                                                         const std::string &layerIdentifier,
                                                         const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription)
        : mapInterface(mapInterface),
          tileInfo(tileInfo),
          layerIdentifier(layerIdentifier),
          layerDescription(layerDescription) {}

bool Tiled2dMapVectorSymbolGroup::initialize(const Tiled2dMapVectorTileInfo::FeatureTuple &feature) {
    const auto &[context, geometry] = feature;

    const auto evalContext = EvaluationContext(tileInfo.zoomIdentifier, context);

    if ((layerDescription->filter != nullptr && !layerDescription->filter->evaluateOr(evalContext, true))) {
        return false;
    }

    auto strongMapInterface = this->mapInterface.lock();
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    if (!strongMapInterface || !camera) {
        return false;
    }

    std::vector<FormattedStringEntry> text = layerDescription->style.getTextField(evalContext);

    if (layerDescription->style.getTextTransform(evalContext) == TextTransform::UPPERCASE && text.size() > 2) {
        for (auto &e: text) {
            e.text = TextHelper::uppercase(e.text);
        }
    }

    std::string fullText;
    for (const auto &textEntry: text) {
        fullText += textEntry.text;
    }

    auto anchor = layerDescription->style.getTextAnchor(evalContext);
    const auto &justify = layerDescription->style.getTextJustify(evalContext);
    const auto &placement = layerDescription->style.getTextSymbolPlacement(evalContext);

    const auto &variableTextAnchor = layerDescription->style.getTextVariableAnchor(evalContext);

    if (!variableTextAnchor.empty()) {
        // TODO: evaluate all anchors correctly and choose best one
        // for now the first one is simply used
        anchor = *variableTextAnchor.begin();
    }

    const auto &fontList = layerDescription->style.getTextFont(evalContext);

    const double symbolSpacingPx = layerDescription->style.getSymbolSpacing(evalContext);
    const double tilePixelFactor = (0.0254 / camera->getScreenDensityPpi()) * tileInfo.zoomLevel;
    const double symbolSpacingMeters = symbolSpacingPx * tilePixelFactor;

    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolObject>> symbolObjects;

    if (context.geomType != vtzero::GeomType::POINT) {

        // TODO: find meaningful way
        // to distribute along line
        double distance = 0;
        double totalDistance = 0;

        bool wasPlaced = false;

        std::vector<Coord> line = {};
        for (const auto &points: geometry.getPointCoordinates()) {
            for (auto &p: points) {
                line.push_back(p);
            }
        }

        for (const auto &points: geometry.getPointCoordinates()) {

            for (auto pointIt = points.begin(); pointIt != points.end(); pointIt++) {
                auto point = *pointIt;

                if (pointIt != points.begin()) {
                    auto last = std::prev(pointIt);
                    double addDistance = Vec2DHelper::distance(Vec2D(last->x, last->y), Vec2D(point.x, point.y));
                    distance += addDistance;
                    totalDistance += addDistance;
                }


                auto pos = getPositioning(pointIt, points);

                if (distance > symbolSpacingMeters && pos) {

                    auto position = pos->centerPosition;

                    wasPlaced = true;

                    const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, {
                            context,
                            std::make_shared<SymbolInfo>(text,
                                                         position,
                                                         line,
                                                         Font(*fontList.begin()),
                                                         anchor,
                                                         pos->angle,
                                                         justify,
                                                         placement
                            )});
                    if (symbolObject) {
                        symbolObjects.push_back(symbolObject);
                    }


                    distance = 0;
                }
            }
        }

        // if no label was placed, place it in the middle of the line
        if (!wasPlaced) {
            distance = 0;
            for (const auto &points: geometry.getPointCoordinates()) {

                for (auto pointIt = points.begin(); pointIt != points.end(); pointIt++) {
                    auto point = *pointIt;

                    if (pointIt != points.begin()) {
                        auto last = std::prev(pointIt);
                        double addDistance = Vec2DHelper::distance(Vec2D(last->x, last->y), Vec2D(point.x, point.y));
                        distance += addDistance;
                    }

                    auto pos = getPositioning(pointIt, points);

                    if (distance > (totalDistance / 2.0) && !wasPlaced && pos) {

                        auto position = pos->centerPosition;

                        wasPlaced = true;
                        const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, {
                                context,
                                std::make_shared<SymbolInfo>(text,
                                                             position,
                                                             std::nullopt,
                                                             Font(*fontList.begin()),
                                                             anchor,
                                                             pos->angle,
                                                             justify,
                                                             placement)
                        });
                        if (symbolObject) {
                            symbolObjects.push_back(symbolObject);
                        }


                        distance = 0;
                    }
                }
            }
        }
    } else {
        for (const auto &p: geometry.getPointCoordinates()) {
            auto midP = p.begin() + p.size() / 2;
            std::optional<double> angle = std::nullopt;

            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, {
                    context,
                    std::make_shared<SymbolInfo>(text,
                                                 *midP,
                                                 std::nullopt,
                                                 Font(*fontList.begin()),
                                                 anchor,
                                                 angle,
                                                 justify,
                                                 placement)
            });
            if (symbolObject) {
                symbolObjects.push_back(symbolObject);
            }

        }
    }
    return true;
}

std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> Tiled2dMapVectorSymbolGroup::getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection) {

    double distance = 10;

    Vec2D curPoint(iterator->x, iterator->y);

    auto prev = iterator;
    if (prev == collection.begin()) { return std::nullopt; }

    while (Vec2DHelper::distance(Vec2D(prev->x, prev->y), curPoint) < distance) {
        prev = std::prev(prev);

        if (prev == collection.begin()) { return std::nullopt; }
    }

    auto next = iterator;
    if (next == collection.end()) { return std::nullopt; }

    while (Vec2DHelper::distance(Vec2D(next->x, next->y), curPoint) < distance) {
        next = std::next(next);

        if (next == collection.end()) { return std::nullopt; }
    }

    double angle = -atan2( next->y - prev->y, next->x - prev->x ) * ( 180.0 / M_PI );
    auto midpoint = Vec2DHelper::midpoint(Vec2D(prev->x, prev->y), Vec2D(next->x, next->y));
    return Tiled2dMapVectorSymbolSubLayerPositioningWrapper(angle, Coord(next->systemIdentifier, midpoint.x, midpoint.y, next->z));
}

std::shared_ptr<Tiled2dMapVectorSymbolObject>
Tiled2dMapVectorSymbolGroup::createSymbolObject(const Tiled2dMapTileInfo &tileInfo, const std::string &layerIdentifier,
                                                const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                const std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>> &symbolInfo) {

}
