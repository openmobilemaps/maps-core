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

#include "RenderingContextInterface.h"
#include <string>
#include <unordered_map>
#include <vector>

class OpenGlContext : public RenderingContextInterface, std::enable_shared_from_this<OpenGlContext> {
  public:
    OpenGlContext();

    int getProgram(const std::string &name);

    void storeProgram(const std::string &name, int program);

    std::vector<unsigned int> &getTexturePointerArray(const std::string &name, int capacity);

    void cleanAll();

    virtual void onSurfaceCreated() override;

    virtual void setViewportSize(const ::Vec2I &size) override;

    virtual ::Vec2I getViewportSize() override;

    virtual void setBackgroundColor(const ::Color &color) override;

    virtual void setupDrawFrame() override;

    virtual void preRenderStencilMask() override;

    virtual void postRenderStencilMask() override;

    virtual void applyScissorRect(const std::optional<::RectI> &scissorRect) override;

  protected:
    Color backgroundColor = Color(0, 0, 0, 1);

    std::unordered_map<std::string, int> programs;
    std::unordered_map<std::string, std::vector<unsigned int>> texturePointers;

    Vec2I viewportSize = Vec2I(0, 0);
};
