/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "OpenGlContext.h"
#include "OpenGlRenderTarget.h"
#include "opengl_wrapper.h"

OpenGlContext::OpenGlContext()
        : programs(), timeCreation(chronoutil::getCurrentTimestamp()) {}

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

    for (const auto &target : renderTargets) {
        target.second->setup(size);
    }
}

::Vec2I OpenGlContext::getViewportSize() { return viewportSize; }

void OpenGlContext::setBackgroundColor(const Color &color) {
    backgroundColor = color;
    backgroundColorValid.clear();
}

void OpenGlContext::setupDrawFrame() {
    if (!backgroundColorValid.test_and_set()) {
        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    }
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    timeFrameDelta = (chronoutil::getCurrentTimestamp() - timeCreation).count();
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

/*not-null*/ std::shared_ptr<OpenGlRenderTargetInterface> OpenGlContext::getCreateRenderTarget(const std::string & name, ::TextureFilterType textureFilter) {
    const auto &targetEntry = renderTargets.find(name);
    std::shared_ptr<OpenGlRenderTargetInterface> renderTarget = targetEntry != renderTargets.end() ? targetEntry->second : nullptr;
    if (renderTarget == nullptr) {
        renderTarget = std::make_shared<OpenGlRenderTarget>(textureFilter);
        renderTargets[name] = renderTarget;

        if (viewportSize.x > 0 && viewportSize.y > 0) {
            renderTarget->setup(viewportSize);
        }
    }
    return renderTarget;
}

void OpenGlContext::deleteRenderTarget(const std::string & name) {
    const auto &target = renderTargets.find(name);
    if (target != renderTargets.end()) {
        target->second->clear();
        renderTargets.erase(target);
    }
}

std::vector</*not-null*/ std::shared_ptr<OpenGlRenderTargetInterface>> OpenGlContext::getRenderTargets() {
    std::vector<std::shared_ptr<OpenGlRenderTargetInterface>> targets;
    for (const auto &entry : renderTargets) {
        targets.push_back(entry.second);
    }
    return targets;
}


void OpenGlContext::resume() {
    for (const auto &target : renderTargets) {
        target.second->setup(viewportSize);
    }
}

void OpenGlContext::pause() {
    for (const auto &target : renderTargets) {
        target.second->clear();
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

void OpenGlContext::storeProgram(const std::string &name, int program) { programs[name] = program; }

void OpenGlContext::cleanAll() {
    for (const auto &target : renderTargets) {
        target.second->clear();
    }

    for (const auto &program : programs) {
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
