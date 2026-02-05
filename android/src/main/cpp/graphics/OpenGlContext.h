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

#include "OpenGlRenderingContextInterface.h"
#include "OpenGlRenderTargetInterface.h"
#include "RenderingContextInterface.h"
#include "RenderingCullMode.h"
#include <chrono>
#include <opengl_wrapper.h>
#include <memory>
#include <string>
#include <unordered_map>

class OpenGlContext
        : public RenderingContextInterface,
          public OpenGlRenderingContextInterface,
          public std::enable_shared_from_this<OpenGlContext> {
public:
    OpenGlContext();

    // RenderingContextInterface

    virtual void onSurfaceCreated() override;

    virtual void setViewportSize(const ::Vec2I &size) override;

    virtual ::Vec2I getViewportSize() override;

    virtual void setBackgroundColor(const ::Color &color) override;

    void setCulling(RenderingCullMode mode) override;

    virtual void setupDrawFrame(int64_t vpMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) override;

    virtual void preRenderStencilMask() override;

    virtual void postRenderStencilMask() override;

    virtual void applyScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual std::shared_ptr<OpenGlRenderingContextInterface> asOpenGlRenderingContext() override;

    // GlRenderingContextInterface

    virtual void resume() override;

    virtual void pause() override;

    virtual /*not-null*/ std::shared_ptr<OpenGlRenderTargetInterface>
    getCreateRenderTarget(const std::string &name, ::TextureFilterType textureFilter, const ::Color &clearColor, bool usesDepthStencil) override;

    virtual void deleteRenderTarget(const std::string &name) override;

    virtual std::vector</*not-null*/ std::shared_ptr<OpenGlRenderTargetInterface>> getRenderTargets() override;

    int getProgram(const std::string &name) override;

    void storeProgram(const std::string &name, int program) override;

    virtual float getAspectRatio() override;

    virtual int64_t getDeltaTimeMs() override;

    // OpenGlContext

    void cleanAll();

    GLuint getFrameUniformsBuffer(bool isScreenSpaceCoords);

protected:
    RenderingCullMode cullMode = RenderingCullMode::NONE;
    std::atomic_flag backgroundColorValid = ATOMIC_FLAG_INIT;
    Color backgroundColor = Color(0, 0, 0, 1);

    std::unordered_map<std::string, int> programs;
    std::mutex renderTargetMutex;
    std::unordered_map<std::string, std::shared_ptr<OpenGlRenderTargetInterface>> renderTargets;

    GLuint frameUniformsBuffer = GL_INVALID_INDEX;
    GLuint identityFrameUniformsBuffer = GL_INVALID_INDEX;

    Vec2I viewportSize = Vec2I(0, 0);

    std::chrono::milliseconds timeCreation;
    int64_t timeFrameDelta = 0;
};
