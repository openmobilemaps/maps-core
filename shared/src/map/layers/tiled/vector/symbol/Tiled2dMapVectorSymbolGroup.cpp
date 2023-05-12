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
#include "AlphaInstancedShaderInterface.h"
#include "TextInstancedShaderInterface.h"
#include "Quad2dInstancedInterface.h"
#include "StretchInstancedShaderInterface.h"

Tiled2dMapVectorSymbolGroup::Tiled2dMapVectorSymbolGroup(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                         const Tiled2dMapTileInfo &tileInfo,
                                                         const std::string &layerIdentifier,
                                                         const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription,
                                                         const std::shared_ptr<SpriteData> &spriteData,
                                                         const std::shared_ptr<TextureHolderInterface> &spriteTexture)
: mapInterface(mapInterface),
tileInfo(tileInfo),
layerIdentifier(layerIdentifier),
layerDescription(layerDescription),
fontProvider(fontProvider),
spriteData(spriteData),
spriteTexture(spriteTexture){}

bool Tiled2dMapVectorSymbolGroup::initialize(const std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features) {

    auto strongMapInterface = this->mapInterface.lock();
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    if (!strongMapInterface || !camera) {
        return false;
    }

    const double tilePixelFactor = (0.0254 / camera->getScreenDensityPpi()) * tileInfo.zoomLevel;

    for (auto const &[context, geometry]: *features) {

        const auto evalContext = EvaluationContext(tileInfo.zoomIdentifier, context);

        if ((layerDescription->filter != nullptr && !layerDescription->filter->evaluateOr(evalContext, true))) {
            continue;
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
        const double symbolSpacingMeters = symbolSpacingPx * tilePixelFactor;

        if (context.geomType != vtzero::GeomType::POINT) {

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

                        const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, context, text, fullText, position, line, fontList, anchor, pos->angle, justify, placement);

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
                            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, context, text, fullText, position, std::nullopt, fontList, anchor, pos->angle, justify, placement);
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

                const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, context, text, fullText, *midP, std::nullopt, fontList, anchor, angle, justify, placement);

                if (symbolObject) {
                    symbolObjects.push_back(symbolObject);
                }

            }
        }
    }

    if (symbolObjects.empty()) {
        return false;
    }

    std::sort(symbolObjects.begin(), symbolObjects.end(),
              [](const auto &a, const auto &b) -> bool {
        return a->symbolSortKey < b->symbolSortKey;
    });

    // TODO: make filtering based on collision at zoomLevel tileInfo.zoomIdentifier + 1

    Tiled2dMapVectorSymbolObject::SymbolObjectInstanceCounts instanceCounts {0,0,0};
    int textStyleCount = 0;
    for(auto const object: symbolObjects){
        const auto &counts = object->getInstanceCounts();
        instanceCounts.icons += counts.icons;
        instanceCounts.textCharacters += counts.textCharacters;
        textStyleCount += instanceCounts.textCharacters == 0 ? 0 : 1;
        if(counts.textCharacters != 0 && !fontResult) {
            fontResult = object->getFont();
        }
        instanceCounts.stretchedIcons += counts.stretchedIcons;
    }

    if (instanceCounts.icons != 0) {
        iconInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(strongMapInterface->getShaderFactory()->createAlphaInstancedShader()->asShaderProgramInterface());

        iconInstancedObject->setInstanceCount(instanceCounts.icons);

        iconAlphas.resize(instanceCounts.icons, 0.0);
        iconRotations.resize(instanceCounts.icons, 0.0);
        iconScales.resize(instanceCounts.icons * 2, 0.0);
        iconPositions.resize(instanceCounts.icons * 2, 0.0);
        iconTextureCoordinates.resize(instanceCounts.icons * 4, 0.0);
    }


    if (instanceCounts.stretchedIcons != 0) {
        stretchedInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadStretchedInstanced(strongMapInterface->getShaderFactory()->createStretchInstancedShader()->asShaderProgramInterface());

        stretchedInstancedObject->setInstanceCount(instanceCounts.stretchedIcons);

        stretchedIconAlphas.resize(instanceCounts.stretchedIcons, 0.0);
        stretchedIconRotations.resize(instanceCounts.stretchedIcons, 0.0);
        stretchedIconScales.resize(instanceCounts.stretchedIcons * 2, 0.0);
        stretchedIconPositions.resize(instanceCounts.stretchedIcons * 2, 0.0);
        stretchedIconStretchInfos.resize(instanceCounts.stretchedIcons * 10, 1.0);
        stretchedIconTextureCoordinates.resize(instanceCounts.stretchedIcons * 4, 0.0);
    }

    if (instanceCounts.textCharacters != 0) {
        textInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createTextInstanced(strongMapInterface->getShaderFactory()->createTextInstancedShader()->asShaderProgramInterface());
        textInstancedObject->setInstanceCount(instanceCounts.textCharacters);

        textStyles.resize(textStyleCount * 9, 0.0);
        textStyleIndices.resize(instanceCounts.textCharacters, 0);
        textRotations.resize(instanceCounts.textCharacters, 0.0);
        textScales.resize(instanceCounts.textCharacters * 2, 0.0);
        textPositions.resize(instanceCounts.textCharacters * 2, 0.0);
        textTextureCoordinates.resize(instanceCounts.textCharacters * 4, 0.0);
    }

#ifdef DRAW_TEXT_BOUNDING_BOX
    if (instanceCounts.icons + instanceCounts.stretchedIcons + instanceCounts.textCharacters > 0) {
        auto shader = strongMapInterface->getShaderFactory()->createPolygonGroupShader();
        auto object = strongMapInterface->getGraphicsObjectFactory()->createPolygonGroup(shader->asShaderProgramInterface());
        boundingBoxLayerObject = std::make_shared<PolygonGroup2dLayerObject>(strongMapInterface->getCoordinateConverterHelper(), object, shader);
        boundingBoxLayerObject->setStyles({
            PolygonStyle(Color(0,0,0,0.3), 0.3),
            PolygonStyle(Color(1,0,0,0.1), 1.0)
        });
    }
#endif

    setupObjects();

    return true;
}

void Tiled2dMapVectorSymbolGroup::setupObjects() {
    const auto context = mapInterface.lock()->getRenderingContext();

    int iconOffset = 0;
    int stretchedIconOffset = 0;
    int textOffset = 0;
    uint16_t textStyleOffset = 0;

    for(auto const object: symbolObjects) {
        object->setupIconProperties(iconPositions, iconTextureCoordinates, iconOffset, tileInfo.zoomIdentifier, spriteTexture, spriteData);
        object->setupStretchIconProperties(stretchedIconPositions, stretchedIconTextureCoordinates, stretchedIconOffset, tileInfo.zoomIdentifier, spriteTexture, spriteData);
        object->setupTextProperties(textTextureCoordinates, textStyleIndices, textOffset, textStyleOffset, tileInfo.zoomIdentifier);
    }

#ifdef DRAW_TEXT_BOUNDING_BOX
    if (boundingBoxLayerObject) {
        boundingBoxLayerObject->getPolygonObject()->setup(mapInterface.lock()->getRenderingContext());
    }
#endif

    if (spriteTexture && spriteData && iconInstancedObject) {
        iconInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
        iconInstancedObject->setPositions(SharedBytes((int64_t)iconPositions.data(), (int32_t)iconAlphas.size(), 2 * (int32_t)sizeof(float)));
        iconInstancedObject->setTextureCoordinates(SharedBytes((int64_t)iconTextureCoordinates.data(), (int32_t)iconAlphas.size(), 4 * (int32_t)sizeof(float)));
        iconInstancedObject->loadTexture(context, spriteTexture);
        iconInstancedObject->asGraphicsObject()->setup(context);
    }

    if (spriteTexture && spriteData && stretchedInstancedObject) {
        stretchedInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
        stretchedInstancedObject->setPositions(SharedBytes((int64_t)stretchedIconPositions.data(), (int32_t)stretchedIconAlphas.size(), 2 * (int32_t)sizeof(float)));
        stretchedInstancedObject->setTextureCoordinates(SharedBytes((int64_t)stretchedIconTextureCoordinates.data(), (int32_t)stretchedIconAlphas.size(), 4 * (int32_t)sizeof(float)));
        stretchedInstancedObject->loadTexture(context, spriteTexture);
        stretchedInstancedObject->asGraphicsObject()->setup(context);
    }

    if (textInstancedObject) {
        textInstancedObject->loadTexture(context, fontResult->imageData);
        textInstancedObject->setTextureCoordinates(SharedBytes((int64_t)textTextureCoordinates.data(), (int32_t)textRotations.size(), 4 * (int32_t)sizeof(float)));
        textInstancedObject->setStyleIndices(SharedBytes((int64_t)textStyleIndices.data(), (int32_t)textStyleIndices.size(), 1 * (int32_t)sizeof(uint16_t)));
        textInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
        textInstancedObject->asGraphicsObject()->setup(context);
    }
}

void Tiled2dMapVectorSymbolGroup::update(const double zoomIdentifier, const double rotation, const double scaleFactor) {
    // icons
    if (!symbolObjects.empty()) {

        int iconOffset = 0;
        int stretchedIconOffset = 0;
        int textOffset = 0;
        uint16_t textStyleOffset = 0;

        for(auto const object: symbolObjects) {
            object->updateIconProperties(iconScales, iconRotations, iconAlphas, iconOffset, zoomIdentifier, scaleFactor, rotation);
            object->updateStretchIconProperties(stretchedIconPositions,stretchedIconScales, stretchedIconRotations, stretchedIconAlphas, stretchedIconStretchInfos, stretchedIconOffset, zoomIdentifier, scaleFactor, rotation);
            object->updateTextProperties(textPositions, textScales, textRotations, textStyles, textOffset, textStyleOffset, zoomIdentifier, scaleFactor, rotation);
        }

        if (iconInstancedObject) {
            iconInstancedObject->setAlphas(SharedBytes((int64_t)iconAlphas.data(), (int32_t)iconAlphas.size(), (int32_t)sizeof(float)));
            iconInstancedObject->setScales(SharedBytes((int64_t)iconScales.data(), (int32_t)iconAlphas.size(), 2 * (int32_t)sizeof(float)));
            iconInstancedObject->setRotations(SharedBytes((int64_t)iconRotations.data(), (int32_t)iconAlphas.size(), 1 * (int32_t)sizeof(float)));
        }

        if (stretchedInstancedObject) {
            stretchedInstancedObject->setPositions(SharedBytes((int64_t)stretchedIconPositions.data(), (int32_t)stretchedIconAlphas.size(), 2 * (int32_t)sizeof(float)));
            stretchedInstancedObject->setAlphas(SharedBytes((int64_t)stretchedIconAlphas.data(), (int32_t)stretchedIconAlphas.size(), (int32_t)sizeof(float)));
            stretchedInstancedObject->setScales(SharedBytes((int64_t)stretchedIconScales.data(), (int32_t)stretchedIconAlphas.size(), 2 * (int32_t)sizeof(float)));
            stretchedInstancedObject->setRotations(SharedBytes((int64_t)stretchedIconRotations.data(), (int32_t)stretchedIconAlphas.size(), 1 * (int32_t)sizeof(float)));
            stretchedInstancedObject->setStretchInfos(SharedBytes((int64_t)stretchedIconStretchInfos.data(), (int32_t)stretchedIconAlphas.size(), 10 * (int32_t)sizeof(float)));
        }

        if (textInstancedObject) {
            textInstancedObject->setPositions(SharedBytes((int64_t)textPositions.data(), (int32_t)textRotations.size(), 2 * (int32_t)sizeof(float)));
            textInstancedObject->setStyles(SharedBytes((int64_t)textStyles.data(), (int32_t)textStyles.size() / 9, 9 * (int32_t)sizeof(float)));
            textInstancedObject->setScales(SharedBytes((int64_t)textScales.data(), (int32_t)textRotations.size(), 2 * (int32_t)sizeof(float)));
            textInstancedObject->setRotations(SharedBytes((int64_t)textRotations.data(), (int32_t)textRotations.size(), 1 * (int32_t)sizeof(float)));
        }

#ifdef DRAW_TEXT_BOUNDING_BOX

        if (boundingBoxLayerObject) {
            std::vector<std::tuple<std::vector<::Coord>, int>> vertices;
            std::vector<int32_t> indices;

            int32_t currentVerticeIndex = 0;
            for (const auto &object: symbolObjects) {
                const auto &combinedBox = object->getCombinedBoundingBox();
                if (combinedBox) {
                    vertices.push_back({
                        std::vector<::Coord> {
                            combinedBox->topLeft,
                            Coord(combinedBox->topLeft.systemIdentifier, combinedBox->bottomRight.x, combinedBox->topLeft.y, 0),
                            combinedBox->bottomRight,
                            Coord(combinedBox->topLeft.systemIdentifier, combinedBox->topLeft.x, combinedBox->bottomRight.y, 0)
                        },
                        object->collides ? 1 : 0
                    });
                    indices.push_back(currentVerticeIndex);
                    indices.push_back(currentVerticeIndex + 1);
                    indices.push_back(currentVerticeIndex + 2);


                    indices.push_back(currentVerticeIndex);
                    indices.push_back(currentVerticeIndex + 3);
                    indices.push_back(currentVerticeIndex + 2);

                    currentVerticeIndex += 4;
                }
            }

            boundingBoxLayerObject->setVertices(vertices, indices);
        }
#endif
    }
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
Tiled2dMapVectorSymbolGroup::createSymbolObject(const Tiled2dMapTileInfo &tileInfo,
                                                const std::string &layerIdentifier,
                                                const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                const FeatureContext &featureContext,
                                                const std::vector<FormattedStringEntry> &text,
                                                const std::string &fullText,
                                                const ::Coord &coordinate,
                                                const std::optional<std::vector<Coord>> &lineCoordinates,
                                                const std::vector<std::string> &fontList,
                                                const Anchor &textAnchor,
                                                const std::optional<double> &angle,
                                                const TextJustify &textJustify,
                                                const TextSymbolPlacement &textSymbolPlacement) {
    return std::make_shared<Tiled2dMapVectorSymbolObject>(mapInterface, fontProvider, tileInfo, layerIdentifier, description, featureContext, text, fullText, coordinate, lineCoordinates, fontList, textAnchor, angle, textJustify, textSymbolPlacement);
}

void Tiled2dMapVectorSymbolGroup::collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<std::vector<OBB2D>> placements) {
    for(auto const object: symbolObjects) {
        object->collisionDetection(zoomIdentifier, rotation, scaleFactor, placements);
    }
}
