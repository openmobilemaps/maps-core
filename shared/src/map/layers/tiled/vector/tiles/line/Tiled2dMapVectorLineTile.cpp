/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorLineTile.h"
#include "MapCamera2dInterface.h"
#include "RenderObject.h"
#include "LineHelper.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"

Tiled2dMapVectorLineTile::Tiled2dMapVectorLineTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Tiled2dMapTileInfo &tileInfo,
                                                         const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                         const std::shared_ptr<LineVectorLayerDescription> &description)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, tileCallbackInterface), usedKeys(description->getUsedKeys()) {}

void Tiled2dMapVectorLineTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                      const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    featureGroups.clear();
    reusableLineStyles.clear();
    styleHashToGroupMap.clear();
    hitDetection.clear();
    usedKeys = std::move(description->getUsedKeys());
    setVectorTileData(tileData);
}

void Tiled2dMapVectorLineTile::update() {
    if (shaders.empty()) {
        return;
    }
    
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }
    const double cameraZoom = camera->getZoom();
     double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(cameraZoom);
    zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.zoomIdentifier);

    const auto zoom = Tiled2dMapVectorRasterSubLayerConfig::getZoomFactorAtIdentifier(floor(zoomIdentifier));
    const auto cameraScalingFactor = camera->asCameraInterface()->getScalingFactor();
    const auto scalingFactor = (cameraScalingFactor / cameraZoom) * zoom;

    for (auto const &line: lines) {
        line->setScalingFactor(scalingFactor);
    }

    auto lineDescription = std::static_pointer_cast<LineVectorLayerDescription>(description);

    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        int i = 0;
        bool needsUpdate = false;
        for (auto const &[key, feature]: featureGroups.at(styleGroupId)) {
            auto const &context = EvaluationContext(zoomIdentifier, feature);
            auto &style = reusableLineStyles.at(styleGroupId).at(i);
            auto normalColor = lineDescription->style.getLineColor(context);
            if (normalColor != style.color.normal) {
                style.color.normal = normalColor;
                needsUpdate = true;
            }
            float opacity = lineDescription->style.getLineOpacity(context);
            if (opacity != style.opacity) {
                style.opacity = opacity * alpha;
                needsUpdate = true;
            }
            float blur = lineDescription->style.getLineBlur(context);
            if (blur != style.blur) {
                style.blur = blur;
                needsUpdate = true;
            }
            auto widthType = SizeType::SCREEN_PIXEL;
            if (widthType != style.widthType) {
                style.widthType = widthType;
                needsUpdate = true;
            }
            float width = lineDescription->style.getLineWidth(context);
            if (width != style.width) {
                style.width = width;
                needsUpdate = true;
            }
            auto dashArray = lineDescription->style.getLineDashArray(context);
            if (dashArray != style.dashArray) {
                style.dashArray = dashArray;
                needsUpdate = true;
            }
            auto lineCap = lineDescription->style.getLineCap(context);
            if (lineCap != style.lineCap) {
                style.lineCap = lineCap;
                needsUpdate = true;
            }

            auto offset = lineDescription->style.getLineOffset(context);
            if(offset != style.offset) {
                style.offset = offset;
                needsUpdate = true;
            }

            i++;
        }

        if (needsUpdate) {
            shaders.at(styleGroupId)->setStyles(reusableLineStyles.at(styleGroupId));
        }
    }


}

void Tiled2dMapVectorLineTile::clear() {
    for (auto const &line: lines) {
        if (line->getLineObject()->isReady()) line->getLineObject()->clear();
    }
}

void Tiled2dMapVectorLineTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }
    const auto &context = mapInterface->getRenderingContext();
    for (auto const &line: lines) {
        auto const &lineObject = line->getLineObject();
        if (!lineObject->isReady()) lineObject->setup(context);
    }
    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorLineTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {

    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }

    if (!tileData->empty()) {
        std::unordered_map<int, int> subGroupCoordCount;
        std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> styleGroupNewLinesMap;
        std::unordered_map<int, std::vector<std::tuple<std::vector<Coord>, int>>> styleGroupLineSubGroupMap;

        bool anyInteractable = false;

        for (auto featureIt = tileData->rbegin(); featureIt != tileData->rend(); ++featureIt) {
            const FeatureContext &featureContext = std::get<0>(*featureIt);
            EvaluationContext evalContext = EvaluationContext(tileInfo.zoomIdentifier, featureContext);
            if ((description->filter == nullptr || description->filter->evaluateOr(evalContext, true))) {
                int styleGroupIndex = -1;
                int styleIndex = -1;
                {
                    auto const hash = featureContext.getStyleHash(usedKeys);

                    auto indexPair = styleHashToGroupMap.find(hash);
                    if (indexPair != styleHashToGroupMap.end()) {
                        styleGroupIndex = indexPair->second.first;
                        styleIndex = indexPair->second.second;
                    }

                    if (styleIndex == -1) {
                        auto reusableStyle = LineStyle(ColorStateList(Color(0.0, 0.0, 0.0, 0.0), Color(0.0, 0.0, 0.0, 0.0)),
                                          ColorStateList(Color(0.0, 0.0, 0.0, 0.0), Color(0.0, 0.0, 0.0, 0.0)),
                                          0.0,
                                          0.0,
                                          SizeType::MAP_UNIT,
                                          0.0,
                                          {},
                                          LineCapType::BUTT,
                                          0.0f);
                        if (!featureGroups.empty() && featureGroups.back().size() < maxStylesPerGroup) {
                            styleGroupIndex = (int) featureGroups.size() - 1;
                            styleIndex = (int) featureGroups.back().size();
                            featureGroups.at(styleGroupIndex).push_back({ hash, featureContext });
                            reusableLineStyles.at(styleGroupIndex).push_back(reusableStyle);
                        } else {
                            styleGroupIndex = (int) featureGroups.size();
                            styleIndex = 0;
                            shaders.push_back(shaderFactory->createLineGroupShader());
                            reusableLineStyles.push_back(std::vector<LineStyle>{reusableStyle});
                            featureGroups.push_back(std::vector<std::tuple<size_t, FeatureContext>>{{hash, featureContext}});
                        }
                        styleHashToGroupMap.insert({hash, {styleGroupIndex, styleIndex}});
                    }
                }

                if (subGroupCoordCount.count(styleGroupIndex) == 0) {
                    subGroupCoordCount.try_emplace(styleGroupIndex, 0);
                }

                const VectorTileGeometryHandler &geometryHandler = std::get<1>(*featureIt);
                std::vector<std::vector<::Coord>> lineCoordinatesVector;

                for (const auto &lineCoordinates: geometryHandler.getLineCoordinates()) {
                    if (lineCoordinates.empty()) { continue; }

                    int numCoords = (int)lineCoordinates.size();
                    int coordCount = subGroupCoordCount[styleGroupIndex];
                    if (coordCount + numCoords > maxNumLinePoints
                        && !styleGroupLineSubGroupMap[styleGroupIndex].empty()) {
                        styleGroupNewLinesMap[styleGroupIndex].push_back(styleGroupLineSubGroupMap[styleGroupIndex]);
                        styleGroupLineSubGroupMap[styleGroupIndex] = std::vector<std::tuple<std::vector<Coord>, int>>();
                        subGroupCoordCount[styleGroupIndex] = 0;
                    }

                    styleGroupLineSubGroupMap[styleGroupIndex].push_back({lineCoordinates, std::min(maxStylesPerGroup - 1, styleIndex)});
                    subGroupCoordCount[styleGroupIndex] = (int)subGroupCoordCount[styleGroupIndex] + numCoords;
                    lineCoordinatesVector.push_back(lineCoordinates);
                }

                if (description->isInteractable(evalContext)) {
                    anyInteractable = true;
                    hitDetection.push_back({lineCoordinatesVector, featureContext});;
                }
            }
        }

        for (const auto &[groupIndex, lineSubGroup] : styleGroupLineSubGroupMap) {
            if (!lineSubGroup.empty() && subGroupCoordCount[groupIndex] > 0) styleGroupNewLinesMap[groupIndex].push_back(lineSubGroup);
        }

        if (anyInteractable) {
            tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsInteractable, description->identifier);
        }

        addLines(styleGroupNewLinesMap);
    } else {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
    }
}


void Tiled2dMapVectorLineTile::addLines(const std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> &styleIdLinesMap) {
    if (styleIdLinesMap.empty()) {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    const auto &objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    const auto &coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!objectFactory || !coordinateConverterHelper) {
        return;
    }

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> lineGroupObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (const auto &[styleGroupIndex, styleLines] : styleIdLinesMap) {
        for (const auto &lineSubGroup: styleLines) {
            auto lineGroupGraphicsObject = objectFactory->createLineGroup(shaders.at(styleGroupIndex)->asShaderProgramInterface());

            auto lineGroupObject = std::make_shared<LineGroup2dLayerObject>(coordinateConverterHelper,
                                                                            lineGroupGraphicsObject,
                                                                            shaders.at(styleGroupIndex));
            lineGroupObject->setLines(lineSubGroup);

            lineGroupObjects.push_back(lineGroupObject);
            newGraphicObjects.push_back(lineGroupGraphicsObject->asGraphicsObject());
        }
    }

    lines = lineGroupObjects;

#ifdef __APPLE__
    setupLines(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorLineTile::setupLines, newGraphicObjects);
#endif

}

void Tiled2dMapVectorLineTile::setupLines(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects) {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (auto const &line: newLineGraphicsObjects) {
        if (!line->isReady()) line->setup(renderingContext);
    }


    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}


std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorLineTile::generateRenderObjects() {
    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;

    for (auto const &object : lines) {
        for (const auto &config : object->getRenderConfig()) {
            newRenderObjects.push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    return newRenderObjects;
}

bool Tiled2dMapVectorLineTile::onClickConfirmed(const Vec2F &posScreen) {
    const auto mapInterface = this->mapInterface.lock();
    const auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    const auto coordinateConverter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !selectionDelegate || !coordinateConverter) {
        return false;
    }

    auto point = camera->coordFromScreenPosition(posScreen);
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    auto lineDescription = std::static_pointer_cast<LineVectorLayerDescription>(description);

    for (auto const &[lineCoordinateVector, featureContext]: hitDetection) {
        for (auto const &coordinates: lineCoordinateVector) {
            auto lineWidth = lineDescription->style.getLineWidth(EvaluationContext(zoomIdentifier, featureContext));
            if (LineHelper::pointWithin(coordinates, point, lineWidth, coordinateConverter)) {
                selectionDelegate->didSelectFeature(featureContext.getFeatureInfo(), description->identifier, point);
                return true;
            }
        }
    }


    return false;
}
