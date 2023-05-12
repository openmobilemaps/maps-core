/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceSymbolDataManagerNew.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "RenderPass.h"
#include "Matrix.h"
#include "StretchShaderInfo.h"
#include "Quad2dInstancedInterface.h"

Tiled2dMapVectorSourceSymbolDataManagerNew::Tiled2dMapVectorSourceSymbolDataManagerNew(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                                 const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                                 const std::string &source,
                                                                                 const std::shared_ptr<FontLoaderInterface> &fontLoader) :
Tiled2dMapVectorSourceDataManager(vectorLayer, mapDescription, source), fontLoader(fontLoader)
{
    for (const auto &layer: mapDescription->layers) {
        if (layer->getType() == VectorLayerType::symbol && layer->source == source) {
            layerDescriptions.insert({layer->identifier, std::static_pointer_cast<SymbolVectorLayerDescription>(layer)});
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::onAdded(const std::weak_ptr< ::MapInterface> &mapInterface) {
    Tiled2dMapVectorSourceDataManager::onAdded(mapInterface);
    textHelper.setMapInterface(mapInterface);
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::onRemoved() {
    Tiled2dMapVectorSourceDataManager::onRemoved();
    // Clear (on GLThread!) and remove all symbol groups - they're dependant on the mapInterface
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::pause() {
    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
//TODO: implement me
//                symbolGroup->clear();
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::resume() {
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

void Tiled2dMapVectorSourceSymbolDataManagerNew::setAlpha(float alpha) {
    this->alpha = alpha;
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                                                     int32_t legacyIndex,
                                                                     bool needsTileReplace) {
    // TODO: update layer description of symbols
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::onVectorTilesUpdated(const std::string &sourceName,
                                                                   std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    auto mapInterface = this->mapInterface.lock();
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    std::thread::id this_id = std::this_thread::get_id();

  //  LogError << "thread " << this_id <<= " sleeping...\n";

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

            if (!(layerDescriptions[layerIdentifier]->minZoom <= tile->tileInfo.zoomIdentifier &&
                  layerDescriptions[layerIdentifier]->maxZoom >= tile->tileInfo.zoomIdentifier)) {
                continue;
            }

            const auto &dataIt = tile->layerFeatureMaps->find(layer->sourceId);

            std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> layerSymbols;

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
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManagerNew::setupSymbolGroups, toSetup, tilesToRemove);
#endif


    mapInterface->invalidate();
}

std::optional<Actor<Tiled2dMapVectorSymbolGroup>> Tiled2dMapVectorSourceSymbolDataManagerNew::createSymbolGroup(const Tiled2dMapTileInfo &tileInfo,
                                                                                                                   const std::string &layerIdentifier,
                                                                                                                   const std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features) {

    const auto fontProvider = WeakActor(mailbox, weak_from_this()).weakActor<Tiled2dMapVectorFontProvider>();
    auto mailbox = std::make_shared<Mailbox>(mapInterface.lock()->getScheduler());
    Actor<Tiled2dMapVectorSymbolGroup> symbolGroupActor = Actor<Tiled2dMapVectorSymbolGroup>(mailbox, mapInterface, fontProvider, tileInfo, layerIdentifier, layerDescriptions.at(layerIdentifier), spriteData, spriteTexture);
    bool success = symbolGroupActor.unsafe()->initialize(features);
    return success ? symbolGroupActor : std::optional<Actor<Tiled2dMapVectorSymbolGroup>>();
}


void Tiled2dMapVectorSourceSymbolDataManagerNew::setupSymbolGroups(const std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toSetup, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove) {
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
        //TODO: implement me
//        symbolGroup->setup();
    }
    
    pregenerateRenderPasses();
}

std::shared_ptr<FontLoaderResult> Tiled2dMapVectorSourceSymbolDataManagerNew::loadFont(const std::string &fontName) {
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


Quad2dD Tiled2dMapVectorSourceSymbolDataManagerNew::getProjectedFrame(const RectCoord &boundingBox, const float &padding, const std::vector<float> &modelMatrix) {

    auto topLeft = Vec2D(boundingBox.topLeft.x, boundingBox.topLeft.y);
    auto topRight = Vec2D(boundingBox.bottomRight.x, topLeft.y);
    auto bottomRight = Vec2D(boundingBox.bottomRight.x, boundingBox.bottomRight.y);
    auto bottomLeft = Vec2D(topLeft.x, bottomRight.y);

    topLeft.x -= padding;
    topLeft.y -= padding;

    topRight.x += padding;
    topRight.y -= padding;

    bottomLeft.x -= padding;
    bottomLeft.y += padding;

    bottomRight.x += padding;
    bottomRight.y += padding;

    Matrix::multiply(modelMatrix, std::vector<float>{(float)topLeft.x, (float)topLeft.y, 0.0, 1.0}, topLeftProj);
    Matrix::multiply(modelMatrix, std::vector<float>{(float)topRight.x, (float)topRight.y, 0.0, 1.0}, topRightProj);
    Matrix::multiply(modelMatrix, std::vector<float>{(float)bottomRight.x, (float)bottomRight.y, 0.0, 1.0}, bottomRightProj);
    Matrix::multiply(modelMatrix, std::vector<float>{(float)bottomLeft.x, (float)bottomLeft.y, 0.0, 1.0}, bottomLeftProj);

    return Quad2dD(Vec2D(topLeftProj[0], topLeftProj[1]), Vec2D(topRightProj[0], topRightProj[1]), Vec2D(bottomRightProj[0], bottomRightProj[1]), Vec2D(bottomLeftProj[0], bottomLeftProj[1]));
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (!tileSymbolGroupMap.empty()) {
        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManagerNew::setupExistingSymbolWithSprite);
    }
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::setupExistingSymbolWithSprite() {
    auto mapInterface = this->mapInterface.lock();

    if (!mapInterface) {
        return;
    }

    for (const auto &[tile, symbolGroupMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupMap) {
            for (auto &symbolGroup: symbolGroups) {
                symbolGroup.unsafe()->setupObjects();
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::collisionDetection(std::vector<std::string> layerIdentifiers, std::shared_ptr<std::vector<OBB2D>> placements) {
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
        const auto &description = layerDescriptions.at(layerIdentifier);
        if (!(description->minZoom <= zoomIdentifier &&
              description->maxZoom >= zoomIdentifier)) {
            continue;
        }
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

void Tiled2dMapVectorSourceSymbolDataManagerNew::update() {
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
            if (!(description->minZoom <= zoomIdentifier &&
                  description->maxZoom >= zoomIdentifier)) {
                continue;
            }
            for (auto &symbolGroup: symbolGroups) {
                symbolGroup.syncAccess([&zoomIdentifier, &rotation, &scaleFactor](auto group){
                    group->update(zoomIdentifier, rotation, scaleFactor);
                });

            }
        }
    }

    //TODO: this only has to be done after a tile has been added
    pregenerateRenderPasses();
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::pregenerateRenderPasses() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return ;
    }

    std::vector<std::tuple<int32_t, std::shared_ptr<RenderPassInterface>>> renderPasses;

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupsMap) {

            const auto &description = layerDescriptions.at(layerIdentifier);
            if (!(description->minZoom <= zoomIdentifier &&
                  description->maxZoom >= zoomIdentifier)) {
                continue;
            }

            const auto index = layerNameIndexMap.at(layerIdentifier);

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
//                // TODO: get renderpasses from SymbolGroups
//
//                if (
//#ifdef DRAW_COLLIDED_TEXT_BOUNDING_BOXES
//                    true
//#else
//                    !wrapper->collides
//#endif
//                    ) {
//
//                    if (wrapper->symbolGraphicsObject) {
//                        renderObjects.push_back(std::make_shared<RenderObject>(wrapper->symbolGraphicsObject, wrapper->iconModelMatrix));
//                    }
//
//                    const auto & textObject = wrapper->textObject->getTextObject();
//                    if (textObject) {
//                        renderObjects.push_back(std::make_shared<RenderObject>(textObject->asGraphicsObject(), wrapper->modelMatrix));
//#ifdef DRAW_TEXT_BOUNDING_BOXES
//                    renderObjects.push_back(std::make_shared<RenderObject>(wrapper->boundingBox->asGraphicsObject(), wrapper->modelMatrix));
//#endif
//                    } else {
//#ifdef DRAW_TEXT_BOUNDING_BOXES
//                    renderObjects.push_back(std::make_shared<RenderObject>(wrapper->boundingBox->asGraphicsObject(), wrapper->iconModelMatrix));
//#endif
//                    }
//                }
//            }
            renderPasses.emplace_back(index, std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects));
        }
    }

    vectorLayer.syncAccess([source = this->source, &renderPasses](const auto &layer){
        if(auto strong = layer.lock()) {
            strong->onRenderPassUpdate(source, true, renderPasses);
        }
    });
}

bool Tiled2dMapVectorSourceSymbolDataManagerNew::onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManagerNew::onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface.lock() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    
    if (!camera || !conversionHelper || !selectionDelegate) {
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
            for (const auto &symbolGroup : symbolGroups) {
                // TODO: execute hit detection in the symbol groups
//                const auto &featureInfoCoordsTuple = symbolGroup->onClickConfirmed(tinyClickBox);
//                if (featureInfoCoordsTuple) {
//                    const &[featureInfo, coordinates] = *featureInfoCoordsTuple;
//                    selectionDelegate->didSelectFeature(featureInfo, symbolGroup->layerIdentifier, coordinates);
//                    return true;
//                }
            }
        }
    }
    return false;

}

bool Tiled2dMapVectorSourceSymbolDataManagerNew::onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManagerNew::onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManagerNew::onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1,
                                                          const Vec2F &posScreen2) {
    return false;
}

void Tiled2dMapVectorSourceSymbolDataManagerNew::clearTouch() {

}
