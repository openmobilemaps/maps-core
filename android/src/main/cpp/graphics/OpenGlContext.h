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
#include "RenderingCullMode.h"
#include <memory>
#include <string>
#include <unordered_map>
#include "ChronoUtil.h"

class OpenGlContext : public RenderingContextInterface, public std::enable_shared_from_this<OpenGlContext> {
  public:
    OpenGlContext();

    int getProgram(const std::string &name);

    void storeProgram(const std::string &name, int program);

    void cleanAll();

    virtual void onSurfaceCreated() override;

    virtual void setViewportSize(const ::Vec2I &size) override;

    virtual ::Vec2I getViewportSize() override;

    virtual void setBackgroundColor(const ::Color &color) override;

    void setCulling(RenderingCullMode mode) override;

    virtual void setupDrawFrame() override;

    virtual void preRenderStencilMask() override;

    virtual void postRenderStencilMask() override;

    virtual void applyScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual float getAspectRatio();

    virtual long getDeltaTimeMs();

  protected:
    RenderingCullMode cullMode = RenderingCullMode::NONE;
    std::atomic_flag backgroundColorValid = ATOMIC_FLAG_INIT;
    Color backgroundColor = Color(0, 0, 0, 1);

    std::unordered_map<std::string, int> programs;

    Vec2I viewportSize = Vec2I(0, 0);

    std::chrono::milliseconds timeCreation;
    long timeFrameDelta = 0;
};
