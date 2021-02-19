/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GraphicsObjectFactoryOpenGl.h"
#include "Line2dOpenGl.h"
#include "Polygon2dOpenGl.h"
#include "Rectangle2dOpenGl.h"

std::shared_ptr<Rectangle2dInterface>
GraphicsObjectFactoryOpenGl::createRectangle(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Rectangle2dOpenGl>(shader);
}

std::shared_ptr<Line2dInterface>
GraphicsObjectFactoryOpenGl::createLine(const std::shared_ptr<::LineShaderProgramInterface> &lineShader) {
    return std::make_shared<Line2dOpenGl>(lineShader);
}

std::shared_ptr<Polygon2dInterface>
GraphicsObjectFactoryOpenGl::createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Polygon2dOpenGl>(shader);
}
