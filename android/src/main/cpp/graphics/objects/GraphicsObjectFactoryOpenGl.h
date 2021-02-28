/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSDK_GRAPHICSOBJECTFACTORYOPENGL_H
#define MAPSDK_GRAPHICSOBJECTFACTORYOPENGL_H

#include "GraphicsObjectFactoryInterface.h"

class GraphicsObjectFactoryOpenGl : public GraphicsObjectFactoryInterface {

    virtual std::shared_ptr<Quad2dInterface> createQuad(const std::shared_ptr<::ShaderProgramInterface> &shader) override;

    virtual std::shared_ptr<Line2dInterface> createLine(const std::shared_ptr<::LineShaderProgramInterface> &lineShader) override;

    virtual std::shared_ptr<Polygon2dInterface> createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) override;
};

#endif // MAPSDK_GRAPHICSOBJECTFACTORYOPENGL_H
