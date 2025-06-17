/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "OpenGlRenderTarget.h"
#include "Logger.h"

OpenGlRenderTarget::OpenGlRenderTarget(::TextureFilterType textureFilter, const ::Color &clearColor) : textureFilter(textureFilter), clearColor(clearColor) {}

// RenderTargetInterface

std::shared_ptr<OpenGlRenderTargetInterface> OpenGlRenderTarget::asGlRenderTargetInterface() {
    return shared_from_this();
}

// OpenGlRenderTargetInterface

std::shared_ptr<RenderTargetInterface> OpenGlRenderTarget::asRenderTargetInterface() {
    return shared_from_this();
}

void OpenGlRenderTarget::setup(const Vec2I &size) {
    std::lock_guard<std::mutex> lock(mutex);
    if (isSetup && (size.x != this->size.x || size.y != this->size.y)) {
        clear();
    }

    if (!isSetup) {
        glGenFramebuffers(1, &framebufferId);
        glGenTextures(1, &textureId);
        this->size = size;

        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        GLint filterParam = textureFilter == TextureFilterType::LINEAR ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);

        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            isSetup = true;
        } else {
            LogError << "Setup of framebuffer (w=" << size.x << ", h=" << size.y <<= ") failed!";
            clear();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void OpenGlRenderTarget::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    if (isSetup) {
        glDeleteTextures(1, &textureId);
        glDeleteFramebuffers(1, &framebufferId);
        isSetup = false;
    }
}

void OpenGlRenderTarget::bindFramebuffer() {
    std::lock_guard<std::mutex> lock(mutex);

    // Get current clear color
    glGetFloatv(GL_COLOR_CLEAR_VALUE, tempColorBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGlRenderTarget::unbindFramebuffer() {
    std::lock_guard<std::mutex> lock(mutex);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Restore previous clear color
    glClearColor(tempColorBuffer[0], tempColorBuffer[1],
                 tempColorBuffer[2], tempColorBuffer[3]);
}

int32_t OpenGlRenderTarget::getTextureId() {
    return (int32_t) textureId;
}
