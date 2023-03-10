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
#include "Tiled2dMapVectorLayer.h"
#include "RenderPass.h"
#include "MapCamera2dInterface.h"
#include "RenderObject.h"
#include "LineHelper.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"

Tiled2dMapVectorLineTile::Tiled2dMapVectorLineTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Tiled2dMapTileInfo &tileInfo,
                                                         const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                         const std::shared_ptr<LineVectorLayerDescription> &description)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, vectorLayer) {
    usedKeys = std::move(description->getUsedKeys());
}

void Tiled2dMapVectorLineTile::updateLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                      const Tiled2dMapVectorTileDataVariant &tileData) {
    Tiled2dMapVectorTile::updateLayerDescription(description, tileData);
    featureGroups.clear();
    reusableLineStyles.clear();
    styleHashToGroupMap.clear();
    hitDetection.clear();
    usedKeys = std::move(description->getUsedKeys());
    setTileData(tileMask, tileData);
}

void Tiled2dMapVectorLineTile::update() {
    if (shaders.empty()) {
        return;
    }
    
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(mapInterface->getCamera()->getZoom());

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


            i++;
        }

        if (needsUpdate) {
            shaders.at(styleGroupId)->setStyles(reusableLineStyles.at(styleGroupId));
        }
    }
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorLineTile::buildRenderPasses() {
    return renderPasses;
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
    vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
}

void Tiled2dMapVectorLineTile::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
    preGenerateRenderPasses();
}

void Tiled2dMapVectorLineTile::setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                 const Tiled2dMapVectorTileDataVariant &tileData) {

    Tiled2dMapVectorTileDataVector data = std::holds_alternative<Tiled2dMapVectorTileDataVector>(tileData)
                                          ? std::get<Tiled2dMapVectorTileDataVector>(tileData) : Tiled2dMapVectorTileDataVector();

    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }

    if (!data.empty()) {
        std::unordered_map<int, int> subGroupCoordCount;
        std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> styleGroupNewLinesMap;
        std::unordered_map<int, std::vector<std::tuple<std::vector<Coord>, int>>> styleGroupLineSubGroupMap;

        for (auto featureIt = data.rbegin(); featureIt != data.rend(); ++featureIt) {
            const FeatureContext &featureContext = std::get<0>(*featureIt);
            if ((description->filter == nullptr || description->filter->evaluateOr(EvaluationContext(-1, featureContext), true))) {
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
                                          LineCapType::BUTT);
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
                    subGroupCoordCount[styleGroupIndex] = 0;
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

                hitDetection.push_back({lineCoordinatesVector, featureContext});
            }
        }

        for (const auto &[groupIndex, lineSubGroup] : styleGroupLineSubGroupMap) {
            if (!lineSubGroup.empty() && subGroupCoordCount[groupIndex] > 0) styleGroupNewLinesMap[groupIndex].push_back(lineSubGroup);
        }

        this->tileMask = tileMask;
        addLines(styleGroupNewLinesMap);
    } else {
        vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
    }
}

void Tiled2dMapVectorLineTile::updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) {
    this->tileMask = tileMask;
    preGenerateRenderPasses();
}


void Tiled2dMapVectorLineTile::addLines(const std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> &styleIdLinesMap) {
    std::vector<std::shared_ptr<GraphicsObjectInterface>> oldGraphicsObjects;
    for (const auto &polygon : this->lines) {
        oldGraphicsObjects.push_back(polygon->getLineObject());
    }

    if (styleIdLinesMap.empty() && oldGraphicsObjects.empty()) {
        vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
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

    for (const auto &[styleGroupIndex, lines] : styleIdLinesMap) {
        for (const auto &lineSubGroup: lines) {
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
    setupLines(newGraphicObjects, oldGraphicsObjects);
#else
    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorLineTile::setupLines, newGraphicObjects, oldGraphicsObjects);
#endif

    preGenerateRenderPasses();
}

void Tiled2dMapVectorLineTile::setupLines(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects,
                                          const std::vector<std::shared_ptr<GraphicsObjectInterface>> &oldLineGraphicsObjects) {
    for (const auto &line : oldLineGraphicsObjects) {
        if (line->isReady()) line->clear();
    }

    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (auto const &line: newLineGraphicsObjects) {
        if (!line->isReady()) line->setup(renderingContext);
    }

    vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
}


void Tiled2dMapVectorLineTile::preGenerateRenderPasses() {
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;

    if (!tileMask) {
        renderPasses = newRenderPasses;
        return;
    }

    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
    for (auto const &object : lines) {
        for (const auto &config : object->getRenderConfig()) {
            renderPassObjectMap[config->getRenderIndex()].push_back(
                    std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(
                RenderPassConfig(description->renderPassIndex.value_or(passEntry.first)),
                passEntry.second, tileMask);
        renderPass->setScissoringRect(scissorRect);
        newRenderPasses.push_back(renderPass);
    }

    renderPasses = newRenderPasses;
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
                selectionDelegate.message(&Tiled2dMapVectorLayerSelectionInterface::didSelectFeature, featureContext, description, point);
                return true;
            }
        }
    }


    return false;
}
