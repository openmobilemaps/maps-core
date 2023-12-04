/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "MapInterface.h"
#include "ColorShaderInterface.h"
#include "MapConfig.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include "SchedulerInterface.h"
#include "LambdaTask.h"
#include "PolygonPatternGroup2dInterface.h"
#include "MapCamera2dInterface.h"
#include "PolygonGroupShaderInterface.h"

void Tiled2dMapVectorBackgroundSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface, layerIndex);
    std::lock_guard<std::recursive_mutex> lck(mutex);

    this->dpFactor = mapInterface->getCamera()->getScreenDensityPpi() / 160.0;

    auto context = std::make_shared<FeatureContext>(vtzero::GeomType::POINT, FeatureContext::mapType{}, 0);
    auto evalContext = EvaluationContext(0.0, dpFactor, context, featureStateManager);

    std::vector<float> vertices = {
        -1,  1, 0,//A
         1,  1, 0,//B
         1, -1, 0,//C
        -1, -1, 0 //D
    };
    std::vector<uint16_t> indices = {
        0, 1, 2, // ABC
        0, 2, 3, // ACD
    };

    patternName = description->style.getPattern(evalContext);
    if (!patternName.empty()) {
        auto shader = mapInterface->getShaderFactory()->createPolygonPatternGroupShader();

        shader->asShaderProgramInterface()->setBlendMode(description->style.getBlendMode(evalContext));

        auto object = mapInterface->getGraphicsObjectFactory()->createPolygonPatternGroup(shader->asShaderProgramInterface());
        object->asGraphicsObject()->setDebugLabel(description->identifier);
        patternObject = std::make_shared<PolygonPatternGroup2dLayerObject>(mapInterface->getCoordinateConverterHelper(), object, shader);
        
        patternObject->setVertices(vertices, indices);
        patternObject->setOpacities({alpha});

        if (spriteTexture && spriteData) {
            setSprites(spriteData, spriteTexture);
        }
    }

    auto shader = mapInterface->getShaderFactory()->createPolygonGroupShader();
    auto object = mapInterface->getGraphicsObjectFactory()->createPolygonGroup(shader->asShaderProgramInterface());
    object->asGraphicsObject()->setDebugLabel(description->identifier);
    polygonObject = std::make_shared<PolygonGroup2dLayerObject>(mapInterface->getCoordinateConverterHelper(), object, shader);

    auto color = description->style.getColor(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager));
    polygonObject->setStyles({
        PolygonStyle(color, alpha)
    });
    polygonObject->setVertices(vertices, indices);

    std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects {  };

    renderObjects.push_back(std::make_shared<RenderObject>(object->asGraphicsObject(), true));
    if (patternObject) {
        renderObjects.push_back(std::make_shared<RenderObject>(patternObject->getPolygonObject(), true));
    }

    auto renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects );
    renderPasses = {
        renderPass
    };

    std::weak_ptr<Tiled2dMapVectorBackgroundSubLayer> weakSelfPtr = weak_from_this();
    auto scheduler = mapInterface->getScheduler();
    if (!scheduler) {
        return;
    }
    scheduler->addTask(std::make_shared<LambdaTask>(
           TaskConfig("Tiled2dMapVectorBackgroundSubLayer setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (!selfPtr) {
                    return;
                }
                auto mapInterface = selfPtr->mapInterface;

               if (mapInterface && !selfPtr->polygonObject->getPolygonObject()->isReady()) {
                   selfPtr->polygonObject->getPolygonObject()->setup(mapInterface->getRenderingContext());
               }
            }));
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorBackgroundSubLayer::buildRenderPasses() {
    if (patternObject) {
        Vec2I viewportSize = mapInterface->getRenderingContext()->getViewportSize();
        patternObject->setScalingFactor(Vec2F(1.0 / viewportSize.x, 1.0 / viewportSize.y));
    }

    return renderPasses;
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorBackgroundSubLayer::buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) {
    return renderPasses;
}

void Tiled2dMapVectorBackgroundSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
}

void Tiled2dMapVectorBackgroundSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();
    if (polygonObject) {
        polygonObject->getPolygonObject()->clear();
    }
    if (patternObject) {
        patternObject->getPolygonObject()->clear();
    }
}

void Tiled2dMapVectorBackgroundSubLayer::resume() {
    Tiled2dMapVectorSubLayer::resume();

    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    if (polygonObject && !polygonObject->getPolygonObject()->isReady()) {
        polygonObject->getPolygonObject()->setup(renderingContext);
    }
    if (patternObject && !patternObject->getPolygonObject()->isReady()) {
        patternObject->getPolygonObject()->setup(renderingContext);
        if (spriteTexture) {
            patternObject->loadTexture(renderingContext, spriteTexture);
        }
    }
}

void Tiled2dMapVectorBackgroundSubLayer::hide() {
    Tiled2dMapVectorSubLayer::hide();
}

void Tiled2dMapVectorBackgroundSubLayer::show() {
    Tiled2dMapVectorSubLayer::show();
}

std::string Tiled2dMapVectorBackgroundSubLayer::getLayerDescriptionIdentifier() {
    return description->identifier;
}

void Tiled2dMapVectorBackgroundSubLayer::setAlpha(float alpha) {
    Tiled2dMapVectorSubLayer::setAlpha(alpha);

    std::lock_guard<std::recursive_mutex> lck(mutex);
    auto color = description->style.getColor(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager));
    polygonObject->setStyles({
        PolygonStyle(color, alpha)
    });
    if (patternObject) {
        patternObject->setOpacities({
            alpha
        });
    }
}

void Tiled2dMapVectorBackgroundSubLayer::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    std::lock_guard<std::recursive_mutex> lck(mutex);
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;
    if (spriteData && spriteTexture && mapInterface && patternObject && !patternName.empty()) {
        const auto spriteIt = spriteData->sprites.find(patternName);
        if (spriteIt != spriteData->sprites.end()) {
            std::vector<float> textureCoordinates(5);

            textureCoordinates[0] = ((float) spriteIt->second.x + spriteIt->second.width) / spriteTexture->getImageWidth();
            textureCoordinates[1] = ((float) spriteIt->second.y + spriteIt->second.height) / spriteTexture->getImageHeight();
            textureCoordinates[2] = ((float) -spriteIt->second.width) / spriteTexture->getImageWidth();
            textureCoordinates[3] = ((float) -spriteIt->second.height) / spriteTexture->getImageHeight();
            textureCoordinates[4] = spriteIt->second.width + (spriteIt->second.height << 16);

            patternObject->setTextureCoordinates(textureCoordinates);

            std::weak_ptr<Tiled2dMapVectorBackgroundSubLayer> weakSelfPtr = weak_from_this();
            auto scheduler = mapInterface->getScheduler();
            if (!scheduler) {
                return;
            }
            scheduler->addTask(std::make_shared<LambdaTask>(
                                                            TaskConfig("Tiled2dMapVectorBackgroundSubLayer setSprites", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr] {
                                                                auto selfPtr = weakSelfPtr.lock();
                                                                if (!selfPtr || !selfPtr->spriteTexture) {
                                                                    return;
                                                                }
                                                                auto mapInterface = selfPtr->mapInterface;

                                                                selfPtr->patternObject->loadTexture(mapInterface->getRenderingContext(), selfPtr->spriteTexture);

                                                                if (mapInterface && selfPtr->patternObject && !selfPtr->patternObject->getPolygonObject()->isReady()) {

                                                                    selfPtr->patternObject->getPolygonObject()->setup(mapInterface->getRenderingContext());
                                                                }
                                                            }));
        } else {
            LogError << "Unable to find sprite " <<= patternName;
        }
    }
}
