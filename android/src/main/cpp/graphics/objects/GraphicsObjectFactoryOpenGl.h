/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "GraphicsObjectFactoryInterface.h"

class GraphicsObjectFactoryOpenGl : public GraphicsObjectFactoryInterface {

    virtual std::shared_ptr<Quad2dInterface> createQuad(const std::shared_ptr<::ShaderProgramInterface> &shader) override;

    virtual std::shared_ptr<Line2dInterface> createLine(const std::shared_ptr<::ShaderProgramInterface> &Shader) override;

    virtual std::shared_ptr<Polygon2dInterface> createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) override;

    virtual std::shared_ptr<LineGroup2dInterface> createLineGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) override;

    virtual std::shared_ptr<PolygonGroup2dInterface>
    createPolygonGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) override;

    virtual std::shared_ptr<Quad2dInterface> createQuadMask() override;

    virtual std::shared_ptr<Polygon2dInterface> createPolygonMask() override;

    virtual std::shared_ptr<TextInterface> createText(const std::shared_ptr<::ShaderProgramInterface> &shader) override;
};
