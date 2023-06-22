/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "RenderPass.h"
#include "Matrix.h"
#include "StretchShaderInfo.h"
#include "Quad2dInstancedInterface.h"
#include "RenderObject.h"

Tiled2dMapVectorSourceSymbolDataManager::Tiled2dMapVectorSourceSymbolDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                                 const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                                 const std::string &source,
                                                                                 const std::shared_ptr<FontLoaderInterface> &fontLoader,
                                                                                 const WeakActor<Tiled2dMapVectorSource> &vectorSource) :
Tiled2dMapVectorSourceDataManager(vectorLayer, mapDescription, source), fontLoader(fontLoader), vectorSource(vectorSource)
{
    for (const auto &layer: mapDescription->layers) {
        if (layer->getType() == VectorLayerType::symbol && layer->source == source) {
            layerDescriptions.insert({layer->identifier, std::static_pointer_cast<SymbolVectorLayerDescription>(layer)});
            if (layer->isInteractable(EvaluationContext(std::nullopt, std::make_shared<FeatureContext>()))) {
                interactableLayers.insert(layer->identifier);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::onAdded(const std::weak_ptr< ::MapInterface> &mapInterface) {
    Tiled2dMapVectorSourceDataManager::onAdded(mapInterface);
    textHelper.setMapInterface(mapInterface);
}

void Tiled2dMapVectorSourceSymbolDataManager::onRemoved() {
    Tiled2dMapVectorSourceDataManager::onRemoved();
    // Clear (on GLThread!) and remove all symbol groups - they're dependant on the mapInterface
}

void Tiled2dMapVectorSourceSymbolDataManager::pause() {
    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
//TODO: implement me
//                symbolGroup->clear();
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::resume() {
    auto mapInterface = this->mapInterface.lock();
    const auto &context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) {
        return;
    }

    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
//TODO: implement me
//                symbolGroup->setup();
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setAlpha(float alpha) {
    this->alpha = alpha;
    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
               symbolGroup.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSymbolGroup::setAlpha, alpha);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                                                     int32_t legacyIndex,
                                                                     bool needsTileReplace) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    for (const auto &[tile, symbolGroupMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupMap) {
            for(const auto &symbolGroup: symbolGroups) {
                symbolGroup.message(&Tiled2dMapVectorSymbolGroup::resetCollisionCache);
            }
        }
    }

    if (layerDescription->source != source || layerDescription->getType() != VectorLayerType::symbol) {
        return;
    }

    layerDescriptions.erase(layerDescription->identifier);
    layerDescriptions.insert({layerDescription->identifier, std::static_pointer_cast<SymbolVectorLayerDescription>(layerDescription)});

    auto const &currentTileInfos = vectorSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toSetup;
    std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;

    for (const auto &tileData: currentTileInfos) {
        auto tileGroup = tileSymbolGroupMap.find(tileData.tileInfo);
        if (tileGroup == tileSymbolGroupMap.end()) {
            continue;
        }

        tileGroup->second.erase(layerDescription->identifier);

        // If new source of layer is not handled by this manager, continue
        if (layerDescription->source != source) {
            continue;
        }

        const auto &dataIt = tileData.layerFeatureMaps->find(layerDescription->sourceLayer);

        if (dataIt != tileData.layerFeatureMaps->end()) {
            // there is something in this layer to display
            const auto &newSymbolGroup = createSymbolGroup(tileData.tileInfo, layerDescription->identifier, dataIt->second);
            if (newSymbolGroup) {
                tileSymbolGroupMap.at(tileData.tileInfo)[layerDescription->identifier].push_back(*newSymbolGroup);
                toSetup.push_back(*newSymbolGroup);
            }
        }
    }
#ifdef __APPLE__
    setupSymbolGroups(toSetup, tilesToRemove);
#else
    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups, toSetup, tilesToRemove);
#endif


    mapInterface->invalidate();
    
}

void Tiled2dMapVectorSourceSymbolDataManager::onVectorTilesUpdated(const std::string &sourceName,
                                                                   std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    auto mapInterface = this->mapInterface.lock();
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    // Just insert pointers here since we will only access the objects inside this method where we know that currentTileInfos is retained
    std::vector<const Tiled2dMapVectorTileInfo*> tilesToAdd;
    std::vector<const Tiled2dMapVectorTileInfo*> tilesToKeep;

    std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;

    for (const auto &vectorTileInfo: currentTileInfos) {
        if (tileSymbolGroupMap.count(vectorTileInfo.tileInfo) == 0) {
            tilesToAdd.push_back(&vectorTileInfo);
        } else {
            tilesToKeep.push_back(&vectorTileInfo);
        }
    }

    for (const auto &[tileInfo, _]: tileSymbolGroupMap) {
        bool found = false;
        for (const auto &currentTile: currentTileInfos) {
            if (tileInfo == currentTile.tileInfo) {
                found = true;
                break;
            }
        }
        if (!found) {
            tilesToRemove.insert(tileInfo);
        }
    }

    if (tilesToAdd.empty() && tilesToRemove.empty()) {
        return;
    }

    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toSetup;

    for (const auto &tile : tilesToAdd) {
        tileSymbolGroupMap[tile->tileInfo] = {};

        for (const auto &[layerIdentifier, layer]: layerDescriptions) {
            const auto &dataIt = tile->layerFeatureMaps->find(layer->sourceLayer);

            if (dataIt != tile->layerFeatureMaps->end()) {
                // there is something in this layer to display
                const auto &newSymbolGroup = createSymbolGroup(tile->tileInfo, layerIdentifier, dataIt->second);
                if (newSymbolGroup) {
                    tileSymbolGroupMap.at(tile->tileInfo)[layerIdentifier].push_back(*newSymbolGroup);
                    toSetup.push_back(*newSymbolGroup);
                }
            }
        }
    }

#ifdef __APPLE__
    setupSymbolGroups(toSetup, tilesToRemove);
#else
    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups, toSetup, tilesToRemove);
#endif


    mapInterface->invalidate();
}

std::optional<Actor<Tiled2dMapVectorSymbolGroup>> Tiled2dMapVectorSourceSymbolDataManager::createSymbolGroup(const Tiled2dMapTileInfo &tileInfo,
                                                                                                                   const std::string &layerIdentifier,
                                                                                                                   const std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features) {

    const auto fontProvider = WeakActor(mailbox, weak_from_this()).weakActor<Tiled2dMapVectorFontProvider>();
    auto mailbox = std::make_shared<Mailbox>(mapInterface.lock()->getScheduler());
    Actor<Tiled2dMapVectorSymbolGroup> symbolGroupActor = Actor<Tiled2dMapVectorSymbolGroup>(mailbox, mapInterface, fontProvider, tileInfo, layerIdentifier, layerDescriptions.at(layerIdentifier));
    bool success = symbolGroupActor.unsafe()->initialize(features);
    symbolGroupActor.unsafe()->setAlpha(alpha);
    return success ? symbolGroupActor : std::optional<Actor<Tiled2dMapVectorSymbolGroup>>();
}


void Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups(const std::vector<Actor<Tiled2dMapVectorSymbolGroup>> &toSetup, const std::unordered_set<Tiled2dMapTileInfo> &tilesToRemove) {
    auto mapInterface = this->mapInterface.lock();
    auto context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) { return; }

    for (const auto &tile: tilesToRemove) {
        auto tileIt = tileSymbolGroupMap.find(tile);
        if (tileIt != tileSymbolGroupMap.end()) {
            for (const auto &[s, symbolGroups] : tileIt->second) {
                for (const auto &symbolGroup : symbolGroups) {
                    //TODO: implement me
//                    symbolGroup->clear();
                }
            }
            tileSymbolGroupMap.erase(tileIt);
        }
    }

    for (const auto &symbolGroup: toSetup) {
        symbolGroup.syncAccess([&](auto group){
            group->setupObjects(spriteData, spriteTexture);
        });
    }
    
    pregenerateRenderPasses();
}

std::shared_ptr<FontLoaderResult> Tiled2dMapVectorSourceSymbolDataManager::loadFont(const std::string &fontName) {
    if (fontLoaderResults.count(fontName) > 0) {
        return fontLoaderResults.at(fontName);
    } else {
        auto fontResult = std::make_shared<FontLoaderResult>(fontLoader->loadFont(Font(fontName)));
        if (fontResult->status == LoaderStatus::OK && fontResult->fontData && fontResult->imageData) {
            fontLoaderResults.insert({fontName, fontResult});
        }
        return fontResult;
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (!tileSymbolGroupMap.empty()) {
        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupExistingSymbolWithSprite);
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setupExistingSymbolWithSprite() {
    auto mapInterface = this->mapInterface.lock();

    if (!mapInterface) {
        return;
    }

    for (const auto &[tile, symbolGroupMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupMap) {
            for (auto &symbolGroup: symbolGroups) {
                symbolGroup.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSymbolGroup::setupObjects, spriteData, spriteTexture);
            }
        }
    }
    
    pregenerateRenderPasses();
}

void Tiled2dMapVectorSourceSymbolDataManager::collisionDetection(std::vector<std::string> layerIdentifiers, std::shared_ptr<std::vector<OBB2D>> placements) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return;
    }

    double zoom = camera->getZoom();
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(zoom);
    double rotation = -camera->getRotation();
    auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    for (const auto layerIdentifier: layerIdentifiers) {
        for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
            const auto objectsIt = symbolGroupsMap.find(layerIdentifier);
            if (objectsIt != symbolGroupsMap.end()) {
                for (auto &symbolGroup: objectsIt->second) {
                    symbolGroup.syncAccess([&zoomIdentifier, &rotation, &scaleFactor, &placements](auto group){
                        group->collisionDetection(zoomIdentifier, rotation, scaleFactor, placements);
                    });
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return;
    }

    const double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    const double rotation = camera->getRotation();

    const auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupsMap) {
            const auto &description = layerDescriptions.at(layerIdentifier);
            for (auto &symbolGroup: symbolGroups) {
                symbolGroup.syncAccess([&zoomIdentifier, &rotation, &scaleFactor](auto group){
                    group->update(zoomIdentifier, rotation, scaleFactor);
                });
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::pregenerateRenderPasses() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return ;
    }

    std::vector<std::shared_ptr<Tiled2dMapVectorLayer::TileRenderDescription>> renderDescriptions;

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupsMap) {
            const int32_t index = layerNameIndexMap.at(layerIdentifier);

            std::vector<std::shared_ptr< ::RenderObjectInterface>> renderObjects;
            for (const auto &group: symbolGroups) {
                group.syncAccess([&renderObjects](auto group){
                    auto iconObject = group->iconInstancedObject;
                    if (iconObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(iconObject->asGraphicsObject()));
                    }
                    auto stretchIconObject = group->stretchedInstancedObject;
                    if (stretchIconObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(stretchIconObject->asGraphicsObject()));
                    }
                    auto textObject = group->textInstancedObject;
                    if (textObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(textObject->asGraphicsObject()));
                    }

                    auto boundingBoxLayerObject = group->boundingBoxLayerObject;
                    if (boundingBoxLayerObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(boundingBoxLayerObject->getPolygonObject()));
                    }
                });
            }
            renderDescriptions.push_back(std::make_shared<Tiled2dMapVectorLayer::TileRenderDescription>(Tiled2dMapVectorLayer::TileRenderDescription{index, renderObjects, nullptr, false}));
        }
    }

    //TODO: message this
    vectorLayer.syncAccess([source = this->source, &renderDescriptions](const auto &layer){
        if(auto strong = layer.lock()) {
            strong->onRenderPassUpdate(source, true, renderDescriptions);
        }
    });
}

bool Tiled2dMapVectorSourceSymbolDataManager::onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface.lock() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!camera || !conversionHelper || !strongSelectionDelegate) {
        return false;
    }

    Coord clickCoords = camera->coordFromScreenPosition(posScreen);
    Coord clickCoordsRenderCoord = conversionHelper->convertToRenderSystem(clickCoords);

    double clickPadding = camera->mapUnitsFromPixels(16);

    OBB2D tinyClickBox(Quad2dD(Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y + clickPadding),
                               Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y + clickPadding)));

    for(const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups] : symbolGroupsMap) {
            if (interactableLayers.count(layerIdentifier) == 0) {
                continue;
            }
            for (const auto &symbolGroup : symbolGroups) {
                auto result = symbolGroup.syncAccess([&tinyClickBox](auto group){
                    return group->onClickConfirmed(tinyClickBox);
                });
                if (result) {
                    strongSelectionDelegate->didSelectFeature(std::get<1>(*result), layerIdentifier, conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), std::get<0>(*result)));
                    return true;
                }
            }
        }
    }
    return false;

}

bool Tiled2dMapVectorSourceSymbolDataManager::onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1,
                                                          const Vec2F &posScreen2) {
    return false;
}

void Tiled2dMapVectorSourceSymbolDataManager::clearTouch() {

}
