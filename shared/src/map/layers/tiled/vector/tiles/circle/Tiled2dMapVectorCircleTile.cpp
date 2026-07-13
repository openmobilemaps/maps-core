/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorCircleTile.h"

#include "CoordinateSystemIdentifiers.h"
#include "MapCameraInterface.h"
#include "RenderObject.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "Vec2DHelper.h"
#include "Vec2FHelper.h"

Tiled2dMapVectorCircleTile::Tiled2dMapVectorCircleTile(
    const std::weak_ptr<MapInterface> &mapInterface,
    const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
    const Tiled2dMapVersionedTileInfo &tileInfo,
    const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
    const std::shared_ptr<CircleVectorLayerDescription> &description,
    const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
    const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
    : Tiled2dMapVectorTile(mapInterface, vectorLayer, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager)
    , usedKeys(description->getUsedKeys())
    , selectionSizeFactor(description->selectionSizeFactor) {
    isStyleZoomDependant = usedKeys.containsUsedKey(ValueKeys::ZOOM);
    isStyleStateDependant = usedKeys.isStateDependant();
}

void Tiled2dMapVectorCircleTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                              const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    usedKeys = description->getUsedKeys();
    isStyleZoomDependant = usedKeys.containsUsedKey(ValueKeys::ZOOM);
    isStyleStateDependant = usedKeys.isStateDependant();
    selectionSizeFactor = std::static_pointer_cast<CircleVectorLayerDescription>(description)->selectionSizeFactor;
    lastZoom = std::nullopt;
    lastAlpha = std::nullopt;

    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorCircleTile::update));

    tileCallbackInterface.message(
        MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady),
        tileInfo,
        description->identifier,
        WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this()));
}

bool Tiled2dMapVectorCircleTile::update() {
    clearStaleGraphicsObjects();

    if (circles.empty()) {
        return false;
    }

    const auto zoomIdentifier = getZoomIdentifier();
    if (!zoomIdentifier) {
        return false;
    }

    const bool inZoomRange = isInZoomRange(*zoomIdentifier);
    if (inZoomRange != isVisible) {
        isVisible = inZoomRange;
        for (auto const &object : renderObjects) {
            object->setHidden(!inZoomRange);
        }
    }

    if (!inZoomRange) {
        return false;
    }

    bool allGraphicsReady = true;
    for (auto const &entry : circles) {
        allGraphicsReady = allGraphicsReady && entry.circleObject->getGraphicsObject()->isReady();
    }

    if (lastZoom &&
        ((isStyleZoomDependant && *lastZoom == *zoomIdentifier) || !isStyleZoomDependant) &&
        lastAlpha == alpha &&
        !isStyleStateDependant &&
        allGraphicsReady) {
        return false;
    }

    lastZoom = *zoomIdentifier;
    lastAlpha = alpha;
    updateCircleObjects(*zoomIdentifier, true);

    return false;
}

std::optional<double> Tiled2dMapVectorCircleTile::getZoomIdentifier() const {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return std::nullopt;
    }

    const auto cameraZoom = camera->getZoom();
    double zoomIdentifier = layerConfig->getZoomIdentifier(cameraZoom);
    if (!mapInterface->is3d()) {
        // In 2D, overzoomed vector tiles keep using the source tile data, so style evaluation
        // must not go below that tile zoom. In 3D, tile selection already follows the
        // camera-derived zoom identifier.
        zoomIdentifier = std::max(zoomIdentifier, static_cast<double>(tileInfo.tileInfo.zoomIdentifier));
    }

    return zoomIdentifier;
}

bool Tiled2dMapVectorCircleTile::isInZoomRange(double zoomIdentifier) const {
    auto circleDescription = std::static_pointer_cast<CircleVectorLayerDescription>(description);
    return circleDescription->maxZoom >= zoomIdentifier && circleDescription->minZoom <= zoomIdentifier;
}

void Tiled2dMapVectorCircleTile::updateCircleObjects(double zoomIdentifier, bool setupGraphicsObjects) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }
    const auto renderingContext = mapInterface->getRenderingContext();
    auto circleDescription = std::static_pointer_cast<CircleVectorLayerDescription>(description);
    for (auto const &entry : circles) {
        const auto context = EvaluationContext(zoomIdentifier, dpFactor, entry.featureContext, featureStateManager);
        const auto blendMode = circleDescription->style.getBlendMode(context);

        auto radius = circleDescription->style.getCircleRadius(context);
        auto strokeWidth = circleDescription->style.getCircleStrokeWidth(context);
        auto innerRadius = std::max(0.0, radius - (strokeWidth * 0.5));
        auto outerRadius = std::max(innerRadius, radius + (strokeWidth * 0.5));
        auto renderRadius = outerRadius + 0.5;
        auto innerRadiusRatio = renderRadius > 0.0 ? static_cast<float>(innerRadius / renderRadius) : 0.0f;

        auto fillColor = circleDescription->style.getCircleColor(context);
        fillColor.a *= static_cast<float>(circleDescription->style.getCircleOpacity(context) * alpha);

        auto strokeColor = circleDescription->style.getCircleStrokeColor(context);
        strokeColor.a *= static_cast<float>(circleDescription->style.getCircleStrokeOpacity(context) * alpha);
        if (strokeWidth <= 0.0) {
            strokeColor.a = 0.0f;
        }

        entry.circleObject->setBlendMode(blendMode);
        entry.circleObject->setStyle(fillColor, strokeColor, innerRadiusRatio);
        entry.circleObject->setPosition(entry.coord, renderRadius);
        if (setupGraphicsObjects && renderingContext && !entry.circleObject->getGraphicsObject()->isReady()) {
            entry.circleObject->getGraphicsObject()->setup(renderingContext);
        }
    }
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorCircleTile::generateRenderObjects() {
    return renderObjects;
}

void Tiled2dMapVectorCircleTile::clear() {
    for (auto const &entry : circles) {
        entry.circleObject->getGraphicsObject()->clear();
    }
}

void Tiled2dMapVectorCircleTile::clearStaleGraphicsObjects() {
    if (toClear.empty()) {
        return;
    }

    for (auto const &object : toClear) {
        object->clear();
    }
    toClear.clear();
}

void Tiled2dMapVectorCircleTile::setup() {
    setupCircles();
}

void Tiled2dMapVectorCircleTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    if (!circles.empty()) {
        for (auto const &entry : circles) {
            toClear.push_back(entry.circleObject->getGraphicsObject());
        }
        circles.clear();
        renderObjects.clear();
    }

    bool anyInteractable = false;
    std::unordered_map<size_t, bool> filterCache;

    for (auto featureIt = tileData->begin(); featureIt != tileData->end(); ++featureIt) {
        const auto [featureContext, geometryHandler] = *featureIt;

        if (featureContext->geomType != vtzero::GeomType::POINT) {
            continue;
        }

        const auto evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, featureContext, featureStateManager);
        bool inside = true;

        if (description->filter) {
            if (featureContext->hasCustomId) {
                inside = description->filter->evaluateOr(evalContext, false);
            } else {
                const auto hash = featureContext->identifier;
                const auto cacheIt = filterCache.find(hash);
                if (cacheIt != filterCache.end()) {
                    inside = cacheIt->second;
                } else {
                    inside = description->filter->evaluateOr(evalContext, false);
                    filterCache[hash] = inside;
                }
            }
        }

        if (!inside) {
            continue;
        }

        const auto interactable = description->isInteractable(evalContext);
        anyInteractable = anyInteractable || interactable;

        for (auto const &points : geometryHandler->getPointCoordinates()) {
            for (auto const &point : points) {
                auto coord = Vec2DHelper::toCoord(point, tileInfo.tileInfo.bounds.topLeft.systemIdentifier);

                auto circleObject = std::make_shared<Circle2dLayerObject>(mapInterface);
#ifdef DEBUG
                circleObject->getGraphicsObject()->setDebugLabel(description->identifier + "_circle_" + tileInfo.tileInfo.to_string_short());
#endif

                circles.push_back({coord, featureContext, interactable, circleObject});
            }
        }
    }

    if (anyInteractable) {
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsInteractable), description->identifier);
    }

    if (circles.empty()) {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
        return;
    }

    if (const auto zoomIdentifier = getZoomIdentifier()) {
        isVisible = isInZoomRange(*zoomIdentifier);
        if (isVisible) {
            updateCircleObjects(*zoomIdentifier, false);
        }
    }

    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;
    newRenderObjects.reserve(circles.size());

    for (auto const &entry : circles) {
        for (auto const &config : entry.circleObject->getRenderConfig()) {
            auto renderObject = std::make_shared<RenderObject>(config->getGraphicsObject(), config->getMaskingObject());
            renderObject->setHidden(!isVisible);
            newRenderObjects.push_back(renderObject);
        }
    }
    renderObjects = std::move(newRenderObjects);

#ifdef __APPLE__
    setupCircles();
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorCircleTile::setupCircles));
#endif
}

void Tiled2dMapVectorCircleTile::setupCircles() {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (auto const &entry : circles) {
        auto graphicsObject = entry.circleObject->getGraphicsObject();
        if (!graphicsObject->isReady()) {
            graphicsObject->setup(renderingContext);
        }
    }

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorCircleTile::pause() {
    for (auto const &entry : circles) {
        entry.circleObject->getGraphicsObject()->pause();
    }
}

void Tiled2dMapVectorCircleTile::resume() {
    auto mapInterface = this->mapInterface.lock();
    const auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (auto const &entry : circles) {
        if (!entry.circleObject->getGraphicsObject()->isReady()) {
            entry.circleObject->getGraphicsObject()->resume(renderingContext);
        }
    }

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

bool Tiled2dMapVectorCircleTile::onClickConfirmed(const Vec2F &posScreen) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return false;
    }

    auto point = camera->coordFromScreenPosition(posScreen);
    return performClick(point);
}

bool Tiled2dMapVectorCircleTile::performClick(const Coord &coord) {
    auto mapInterface = this->mapInterface.lock();
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    auto strongVectorLayer = vectorLayer.lock();
    if (!strongSelectionDelegate || !strongVectorLayer || !converter || !camera) {
        return false;
    }

    const auto clickedScreenPos = camera->screenPosFromCoord(coord);
    double zoomIdentifier = layerConfig->getZoomIdentifier(camera->getZoom());
    if (!mapInterface->is3d()) {
        zoomIdentifier = std::max(zoomIdentifier, static_cast<double>(tileInfo.tileInfo.zoomIdentifier));
    }

    auto circleDescription = std::static_pointer_cast<CircleVectorLayerDescription>(description);
    const StringInterner &stringTable = strongVectorLayer->getStringInterner();

    std::vector<VectorLayerFeatureInfo> featureInfos;
    for (auto iter = circles.rbegin(); iter != circles.rend(); ++iter) {
        if (!iter->interactable) {
            continue;
        }
        const auto context = EvaluationContext(zoomIdentifier, dpFactor, iter->featureContext, featureStateManager);
        const auto radius = circleDescription->style.getCircleRadius(context);
        const auto strokeWidth = circleDescription->style.getCircleStrokeWidth(context);
        const auto hitRadius = (radius + (strokeWidth * 0.5)) * selectionSizeFactor;
        const auto circleScreenPos = camera->screenPosFromCoord(iter->coord);
        if (Vec2FHelper::distanceSquared(circleScreenPos, clickedScreenPos) > hitRadius * hitRadius) {
            continue;
        }

        const auto featureInfo = iter->featureContext->getFeatureInfo(stringTable);
        if (multiselect) {
            featureInfos.push_back(featureInfo);
        } else if (strongSelectionDelegate->didSelectFeature(
                       featureInfo,
                       description->identifier,
                       converter->convert(CoordinateSystemIdentifiers::EPSG4326(), coord))) {
            return true;
        }
    }

    if (multiselect && !featureInfos.empty()) {
        return strongSelectionDelegate->didMultiSelectLayerFeatures(
            featureInfos,
            description->identifier,
            converter->convert(CoordinateSystemIdentifiers::EPSG4326(), coord));
    }

    return false;
}
