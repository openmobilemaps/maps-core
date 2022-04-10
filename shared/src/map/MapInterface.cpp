/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Matrix.h"
#include "Logger.h"
#include "MapScene.h"
#include "RectCoord.h"
#include "CoordinateSystemIdentifiers.h"

std::shared_ptr<MapInterface> MapInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                                   const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
                                                   const std::shared_ptr<::RenderingContextInterface> &renderingContext,
                                                   const MapConfig &mapConfig,
                                                   const std::shared_ptr<::SchedulerInterface> &scheduler, float pixelDensity) {
    auto scene = SceneInterface::create(graphicsFactory, shaderFactory, renderingContext);
    return std::make_shared<MapScene>(scene, mapConfig, scheduler, pixelDensity);
}

std::shared_ptr<MapInterface> MapInterface::createWithOpenGl(const MapConfig &mapConfig,
                                                             const std::shared_ptr<::SchedulerInterface> &scheduler,
                                                             float pixelDensity) {
#ifdef __ANDROID__
    return std::make_shared<MapScene>(SceneInterface::createWithOpenGl(), mapConfig, scheduler, pixelDensity);
#else
    return nullptr;
#endif
}

RectCoord MapInterface::convertMvpMatrixToViewportBounds(int64_t mvpMatrix_, const ::Vec2I &viewport) {

    std::vector<float> matrix = std::vector<float>(16, 0.0f);
    std::vector<float> mvpMatrix((float *) mvpMatrix_, (float *) mvpMatrix_ + 16);
    bool invertSuccess = Matrix::invertM(matrix, 0, mvpMatrix, 0);

    if (!invertSuccess) {
        LogError <<= "Couldn't invert MVP.";
        return RectCoord(Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0, 0, 0),
                         Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 1, 1, 0));
    }

    auto res = Matrix::multiply(matrix, {-1.0, 1.0, 0.0, 1.0});
    Coord topLeft = Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), res[0] / res[3], res[1] / res[3], 0);

    res = Matrix::multiply(matrix, {1.0, -1.0, 0.0, 1.0});
    Coord bottomRight = Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), res[0] / res[3], res[1] / res[3], 0);


    return RectCoord(topLeft, bottomRight);
}

float MapInterface::convertMvpMatrixToRotation(int64_t mvpMatrix_, const ::Vec2I &viewport) {

    std::vector<float> matrix = std::vector<float>(16, 0.0f);
    std::vector<float> mvpMatrix((float *) mvpMatrix_, (float *) mvpMatrix_ + 16);
    bool invertSuccess = Matrix::invertM(matrix, 0, mvpMatrix, 0);

    if (!invertSuccess) {
        LogError <<= "Couldn't invert MVP.";
        return 0.0f;
    }

    auto res = Matrix::multiply(matrix, {-1.0, 0.0, 0.0, 1.0});
    Coord topLeft = Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), res[0] / res[3], res[1] / res[3], 0);

    res = Matrix::multiply(matrix, {1.0, 0.0, 0.0, 1.0});
    Coord bottomRight = Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), res[0] / res[3], res[1] / res[3], 0);

    return atan2(topLeft.y - bottomRight.y, bottomRight.x - topLeft.x);
}