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
#include "RenderConfigInterface.h"
#include "Logger.h"

OpenGlRenderTarget::OpenGlRenderTarget(::TextureFilterType textureFilter, const ::Color &clearColor, bool usesDepthStencil)
        : textureFilter(textureFilter), clearColor(clearColor), usesDepthStencil(usesDepthStencil) {}

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
        clearGlData();
    }

    if (!isSetup) {
        glGenFramebuffers(1, &framebufferId);
        glGenTextures(1, &textureId);
        if (usesDepthStencil) {
            glGenRenderbuffers(1, &depthStencilBufferId);
        }
        this->size = size;

        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        GLint filterParam = textureFilter == TextureFilterType::LINEAR ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);

        if (usesDepthStencil) {
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        if (usesDepthStencil) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBufferId);
        }

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
    clearGlData();
}

void OpenGlRenderTarget::clearGlData() {
    // Mutex locking by caller
    if (isSetup) {
        glDeleteTextures(1, &textureId);
        glDeleteFramebuffers(1, &framebufferId);
        if (usesDepthStencil) {
            glDeleteRenderbuffers(1, &depthStencilBufferId);
        }
        isSetup = false;
    }
}

void OpenGlRenderTarget::bindFramebuffer(const std::shared_ptr<RenderingContextInterface> &renderingContext) {
    // Lazy setup
    setup(renderingContext->getViewportSize());

    std::lock_guard<std::mutex> lock(mutex);

    // Get current clear color
    glGetFloatv(GL_COLOR_CLEAR_VALUE, tempColorBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    if (usesDepthStencil) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

void OpenGlRenderTarget::unbindFramebuffer() {
    std::lock_guard<std::mutex> lock(mutex);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Restore previous clear color
    glClearColor(tempColorBuffer[0], tempColorBuffer[1],
                 tempColorBuffer[2], tempColorBuffer[3]);
}

int32_t OpenGlRenderTarget::getTextureId() {
    std::lock_guard<std::mutex> lock(mutex);
    return (int32_t) textureId;
}
