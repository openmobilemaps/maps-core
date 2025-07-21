/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IconLayerObject.h"
#include "DoubleAnimation.h"
#include "RasterStyleAnimation.h"
#include "RenderObject.h"
#include <cmath>
#include <cassert>
#include "RectD.h"
#include "IconType.h"
#include "Anchor.h"
#include "MapCameraInterface.h"
#include "MapConfig.h"

IconLayerObject::IconLayerObject(std::shared_ptr<Quad2dInstancedInterface> quad,
                                             const std::shared_ptr<IconInfoInterface> &icon,
                                             const std::shared_ptr<AlphaInstancedShaderInterface> &shader,
                                 const std::shared_ptr<MapInterface> &mapInterface,
                                 bool is3d)
        : quad(quad),
          icon(icon),
          shader(shader),
          mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)),
          graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {
    int count = 1;
    quad->setInstanceCount(count);

    iconAlphas.resize(count, 1.0);
    iconRotations.resize(count, 0.0);
    iconScales.resize(count * 2, 0.0);
    iconPositions.resize(count * (is3d ? 3 : 2), 0.0);
    iconTextureCoordinates = {0.0, 0.0, 1.0, 1.0};
    iconOffsets.resize(count * 2, 0.0);

    auto renderCoord = conversionHelper->convertToRenderSystem(icon->getCoordinate());
    if (is3d) {
        const double sinY = sin(renderCoord.y);
        const double cosY = cos(renderCoord.y);
        const double sinX = sin(renderCoord.x);
        const double cosX = cos(renderCoord.x);

        origin = Vec3D(renderCoord.z * sinY * cosX, renderCoord.z * cosY, -renderCoord.z * sinY * sinX);
    } else {
        origin = Vec3D(renderCoord.x, renderCoord.y, 0.0);
    }

    quad->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), origin, true);
}

void IconLayerObject::setup(const std::shared_ptr<RenderingContextInterface> context) {
    getGraphicsObject()->setup(context);
    getQuadObject()->loadTexture(context, icon->getTexture());

    quad->setTextureCoordinates(SharedBytes((int64_t)iconTextureCoordinates.data(), (int32_t) iconAlphas.size(), 4 * (int32_t)sizeof(float)));
    quad->setAlphas(SharedBytes((int64_t) iconAlphas.data(), (int32_t) iconAlphas.size(), (int32_t) sizeof(float)));
}

void IconLayerObject::update() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;

    if(!context || !camera) { return; }

    auto viewport = context->getViewportSize();

    const auto scaleFactor = camera->getScalingFactor();

    auto iconSize = icon->getIconSize();
    auto type = icon->getType();

    // POSITION

    auto currentRenderCoord = conversionHelper->convertToRenderSystem(icon->getCoordinate());
    if (is3d) {
        const double sinY = sin(currentRenderCoord.y);
        const double cosY = cos(currentRenderCoord.y);
        const double sinX = sin(currentRenderCoord.x);
        const double cosX = cos(currentRenderCoord.x);

        iconPositions = {
            (float)(currentRenderCoord.z * (sinY * cosX) - origin.x),
            (float)(currentRenderCoord.z * cosY - origin.y),
            (float)(-currentRenderCoord.z * (sinY * sinX) - origin.z)
        };

    } else {
        iconPositions = {(float) (currentRenderCoord.x - origin.x), (float) (currentRenderCoord.y - origin.y)};
    }

    // SCALE

    auto width = iconSize.x;
    auto height = iconSize.y;

    if(type == IconType::FIXED || type == IconType::ROTATION_INVARIANT) {
        float meterToMapUnit = mapInterface->getMapConfig().mapCoordinateSystem.unitToScreenMeterFactor;

        float z = meterToMapUnit / scaleFactor;

        if(is3d) {
            iconScales[0] = z * width;
            iconScales[1] = z * height;
            iconScales[0] /= double(viewport.x);
            iconScales[1] /= double(viewport.y);
        } else {
            iconScales[0] = meterToMapUnit * width;
            iconScales[1] = meterToMapUnit * height;
        }
    } else {
        if(is3d) {
            iconScales[0] = 2.0 * width / double(viewport.x);
            iconScales[1] = 2.0 * height / double(viewport.y);
        } else {
            iconScales[0] = width * scaleFactor;
            iconScales[1] = height * scaleFactor;
        }
    }

    // ROTATION

    double angle = 0.0;
    if(type == IconType::ROTATION_INVARIANT || type == IconType::INVARIANT) {
        angle = camera->getRotation();
    }

    iconRotations[0] = angle;

    // OFFSETS

    iconOffsets[0] = 0.0;
    iconOffsets[1] = 0.0;

    const Vec2F &anchor = icon->getIconAnchor();
    float ratioLeftRight = std::clamp(anchor.x, 0.0f, 1.0f);
    float ratioTopBottom = std::clamp(anchor.y, 0.0f, 1.0f);

    // ratio = 0.5 -> center -> nothing to do
    // ratio = 1.0 -> right in middle -> -0.5 * width
    // ratio = 0.0 -> left in middle -> 0.5 * width
    if(!is3d) {
        iconOffsets[0] = (0.5 - ratioLeftRight) * width * scaleFactor;
        iconOffsets[1] = (0.5 - ratioTopBottom) * height * scaleFactor;
    } else {
        iconOffsets[0] = 2.0 * (0.5 - ratioLeftRight) * width / viewport.x;
        iconOffsets[1] = 2.0 * (0.5 - (1.0 - ratioTopBottom)) * height / viewport.y;
    }

    quad->setPositions(SharedBytes((int64_t) iconPositions.data(), (int32_t) iconAlphas.size(),
                                   (int32_t) iconPositions.size() * (int32_t) sizeof(float)));

    quad->setScales(
        SharedBytes((int64_t) iconScales.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));

    quad->setRotations(
        SharedBytes((int64_t) iconRotations.data(), (int32_t) iconAlphas.size(), 1 * (int32_t) sizeof(float)));

    quad->setPositionOffset(
        SharedBytes((int64_t) iconOffsets.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));

    if (animation) {
        animation->update();
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> IconLayerObject::getRenderConfig() { return {renderConfig}; }

void IconLayerObject::setAlpha(float alpha) {
    iconAlphas[0] = alpha;
    quad->setAlphas(SharedBytes((int64_t) iconAlphas.data(), (int32_t) iconAlphas.size(), (int32_t) sizeof(float)));

    mapInterface->invalidate();
}

std::shared_ptr<Quad2dInstancedInterface> IconLayerObject::getQuadObject() { return quad; }

std::shared_ptr<GraphicsObjectInterface> IconLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<RenderObjectInterface> IconLayerObject::getRenderObject() {
    return renderObject;
}

void IconLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration) {
    assert(shader != nullptr);
    std::weak_ptr<IconLayerObject> weakSelf = weak_from_this();
    animation = std::make_shared<DoubleAnimation>(
            duration, startAlpha, targetAlpha, InterpolatorFunction::EaseIn,
            [weakSelf](double alpha) {
                if (auto selfPtr = weakSelf.lock()) {
                    selfPtr->setAlpha(alpha);
                }
            },
            [weakSelf, targetAlpha] {
                if (auto selfPtr = weakSelf.lock()) {
                    selfPtr->setAlpha(targetAlpha);
                    selfPtr->animation = nullptr;
                }
            });
    animation->start();
    mapInterface->invalidate();
}

std::shared_ptr<ShaderProgramInterface> IconLayerObject::getShader() {
    if (shader) {
        return shader->asShaderProgramInterface();
    }
    return nullptr;
}
