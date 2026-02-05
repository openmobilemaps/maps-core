/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ChronoUtil.h"
#include "OpenGlContext.h"
#include "OpenGlRenderTarget.h"
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

OpenGlContext::OpenGlContext()
    : programs()
    , timeCreation(chronoutil::getCurrentTimestamp()) {
#ifdef __EMSCRIPTEN__
    EmscriptenWebGLContextAttributes attrs;
    attrs.majorVersion = 2;
    attrs.explicitSwapControl = false;
    attrs.stencil = true;
    attrs.antialias = true;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#glCanvas", &attrs);
    if (context == 0) {
        return;
    }
    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(context);
    if (res != EMSCRIPTEN_RESULT_SUCCESS) {
        return;
    }
#endif
}

// RenderingContextInterface

void OpenGlContext::onSurfaceCreated() {
    cleanAll();
    setCulling(cullMode);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClearStencil(0);
}

void OpenGlContext::setViewportSize(const ::Vec2I &size) {
    viewportSize = size;
    glViewport(0, 0, size.x, size.y);

    {
        std::lock_guard<std::mutex> lock(renderTargetMutex);
        for (const auto &target: renderTargets) {
            target.second->setup(size);
        }
    }
}

::Vec2I OpenGlContext::getViewportSize() { return viewportSize; }

void OpenGlContext::setBackgroundColor(const Color &color) {
    backgroundColor = color;
    backgroundColorValid.clear();
}

void OpenGlContext::setupDrawFrame(int64_t vpMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) {
    if (!backgroundColorValid.test_and_set()) {
        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    }
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    timeFrameDelta = (chronoutil::getCurrentTimestamp() - timeCreation).count();

    if (frameUniformsBuffer != GL_INVALID_INDEX && identityFrameUniformsBuffer != GL_INVALID_INDEX) {
        BaseShaderProgramOpenGl::setupFrameUniforms(frameUniformsBuffer, identityFrameUniformsBuffer, vpMatrix, origin,
                                                    screenPixelAsRealMeterFactor, timeFrameDelta / 1000.0);
    }
}

void OpenGlContext::preRenderStencilMask() {
    glEnable(GL_STENCIL_TEST);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 128, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void OpenGlContext::postRenderStencilMask() { glDisable(GL_STENCIL_TEST); }

void OpenGlContext::applyScissorRect(const std::optional<::RectI> &scissorRect) {
    if (scissorRect) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorRect->x, scissorRect->y, scissorRect->width, scissorRect->height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void OpenGlContext::setCulling(RenderingCullMode mode) {
    cullMode = mode;
    switch (mode) {
        case RenderingCullMode::FRONT:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case RenderingCullMode::BACK:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case RenderingCullMode::NONE:
            glDisable(GL_CULL_FACE);
            break;
    }
}

std::shared_ptr<OpenGlRenderingContextInterface> OpenGlContext::asOpenGlRenderingContext() {
    return shared_from_this();
}

// OpenGlRenderingContextInterface

/*not-null*/ std::shared_ptr<OpenGlRenderTargetInterface> OpenGlContext::getCreateRenderTarget(const std::string & name, ::TextureFilterType textureFilter, const ::Color & clearColor, bool usesDepthStencil) {
    std::lock_guard<std::mutex> lock(renderTargetMutex);
    const auto &targetEntry = renderTargets.find(name);
    std::shared_ptr<OpenGlRenderTargetInterface> renderTarget = targetEntry != renderTargets.end() ? targetEntry->second : nullptr;
    if (renderTarget == nullptr) {
        renderTarget = std::make_shared<OpenGlRenderTarget>(textureFilter, clearColor, usesDepthStencil);
        renderTargets[name] = renderTarget;
    }
    return renderTarget;
}

void OpenGlContext::deleteRenderTarget(const std::string & name) {
    std::lock_guard<std::mutex> lock(renderTargetMutex);
    const auto &target = renderTargets.find(name);
    if (target != renderTargets.end()) {
        target->second->clear();
        renderTargets.erase(target);
    }
}

std::vector</*not-null*/ std::shared_ptr<OpenGlRenderTargetInterface>> OpenGlContext::getRenderTargets() {
    std::lock_guard<std::mutex> lock(renderTargetMutex);
    std::vector<std::shared_ptr<OpenGlRenderTargetInterface>> targets;
    for (const auto &entry : renderTargets) {
        targets.push_back(entry.second);
    }
    return targets;
}


void OpenGlContext::resume() {
    {
        std::lock_guard<std::mutex> lock(renderTargetMutex);
        for (const auto &target: renderTargets) {
            target.second->setup(viewportSize);
        }
    }

    if (frameUniformsBuffer == GL_INVALID_INDEX) {
        glGenBuffers(1, &frameUniformsBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, frameUniformsBuffer);
        glBufferData(GL_UNIFORM_BUFFER, BaseShaderProgramOpenGl::FRAME_UBO_SIZE, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    if (identityFrameUniformsBuffer == GL_INVALID_INDEX) {
        glGenBuffers(1, &identityFrameUniformsBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, identityFrameUniformsBuffer);
        glBufferData(GL_UNIFORM_BUFFER, BaseShaderProgramOpenGl::FRAME_UBO_SIZE, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void OpenGlContext::pause() {
    {
        std::lock_guard<std::mutex> lock(renderTargetMutex);
        for (const auto &target: renderTargets) {
            target.second->clear();
        }
    }

    if (frameUniformsBuffer != GL_INVALID_INDEX) {
        glDeleteBuffers(1, &frameUniformsBuffer);
        frameUniformsBuffer = GL_INVALID_INDEX;
    }

    if (identityFrameUniformsBuffer != GL_INVALID_INDEX) {
        glDeleteBuffers(1, &identityFrameUniformsBuffer);
        identityFrameUniformsBuffer = GL_INVALID_INDEX;
    }
}

// OpenGlContext

int OpenGlContext::getProgram(const std::string &name) {
    auto p = programs.find(name);
    if (p == programs.end()) {
        return 0;
    } else {
        return p->second;
    }
}

void OpenGlContext::storeProgram(const std::string &name, int program) {
    auto p = programs.find(name);
    if (p != programs.end()) {
        glDeleteProgram(program);
    } else {
        programs[name] = program;
    }
}

void OpenGlContext::cleanAll() {
    {
        std::lock_guard<std::mutex> lock(renderTargetMutex);
        for (const auto &target: renderTargets) {
            target.second->clear();
        }
    }


    for (const auto &program: programs) {
        GLuint programId = program.second;
        glDeleteProgram(programId);
    }

    programs.clear();

}

float OpenGlContext::getAspectRatio() {
    return viewportSize.x / (float) viewportSize.y;
}

int64_t OpenGlContext::getDeltaTimeMs() {
    return timeFrameDelta;
}

GLuint OpenGlContext::getFrameUniformsBuffer(bool isScreenSpaceCoords) {
    return isScreenSpaceCoords ? identityFrameUniformsBuffer : frameUniformsBuffer;
}
