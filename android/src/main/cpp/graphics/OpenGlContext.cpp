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
#include "opengl_wrapper.h"

OpenGlContext::OpenGlContext()
    : programs()
    , texturePointers() {}

int OpenGlContext::getProgram(const std::string &name) {
    auto p = programs.find(name);
    if (p == programs.end()) {
        return 0;
    } else {
        return p->second;
    }
}

void OpenGlContext::storeProgram(const std::string &name, int program) { programs[name] = program; }

std::vector<unsigned int> &OpenGlContext::getTexturePointerArray(const std::string &name, int capacity) {
    if (texturePointers.find(name) == texturePointers.end()) {
        texturePointers[name] = std::vector<unsigned int>(capacity, 0);
    }

    return texturePointers[name];
}

void OpenGlContext::cleanAll() {
    for (std::unordered_map<std::string, int>::iterator it = programs.begin(); it != programs.end(); ++it) {
        glDeleteProgram(it->second);
    }

    programs.clear();

    for (std::unordered_map<std::string, std::vector<unsigned int>>::iterator it = texturePointers.begin();
         it != texturePointers.end(); ++it) {
        glDeleteTextures(GLsizei(it->second.size()), &it->second[0]);
    }
    texturePointers.clear();
}

void OpenGlContext::onSurfaceCreated() {
    cleanAll();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void OpenGlContext::setViewportSize(const ::Vec2I &size) {
    viewportSize = size;
    glViewport(0, 0, size.x, size.y);
}

::Vec2I OpenGlContext::getViewportSize() { return viewportSize; }

void OpenGlContext::setBackgroundColor(const Color &color) { backgroundColor = color; }

void OpenGlContext::setupDrawFrame() {
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearStencil(0);
}

void OpenGlContext::preRenderStencilMask() {
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glClearStencil(0);
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
