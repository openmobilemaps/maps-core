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

#include "OpenGlRenderTargetInterface.h"
#include "RenderTargetInterface.h"
#include "TextureFilterType.h"
#include "Color.h"
#include <opengl_wrapper.h>
#include <mutex>

class OpenGlRenderTarget : public RenderTargetInterface, public OpenGlRenderTargetInterface, public std::enable_shared_from_this<OpenGlRenderTarget> {
public:

    OpenGlRenderTarget(::TextureFilterType textureFilter, const ::Color &clearColor);

    // RenderTargetInterface

    virtual /*nullable*/ std::shared_ptr<OpenGlRenderTargetInterface> asGlRenderTargetInterface() override;

    // OpenGlRenderTargetInterface

    virtual /*not-null*/ std::shared_ptr<RenderTargetInterface> asRenderTargetInterface() override;

    virtual void setup(const Vec2I &size) override;

    virtual void clear() override;

    virtual void bindFramebuffer() override;

    virtual void unbindFramebuffer() override;

    virtual int32_t getTextureId() override;

private:
    const TextureFilterType textureFilter;
    const Color clearColor;

    std::mutex mutex;

    bool isSetup = false;

    GLuint framebufferId;
    GLuint textureId;
    Vec2I size = Vec2I(0, 0);

    GLfloat tempColorBuffer[4];
};
