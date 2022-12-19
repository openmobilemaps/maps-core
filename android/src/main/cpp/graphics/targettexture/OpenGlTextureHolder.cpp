/*
 * Copyright (c) 2022 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "OpenGlTextureHolder.h"

OpenGlTextureHolder::OpenGlTextureHolder() {

    // The texture we're going to render to
    glGenTextures(1, &renderedTexture);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, renderedTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 768, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
}

int32_t OpenGlTextureHolder::getImageWidth() {
    return 1024;
}

int32_t OpenGlTextureHolder::getImageHeight() {
    return 768;
}

int32_t OpenGlTextureHolder::getTextureWidth() {
    return 1024;
}

int32_t OpenGlTextureHolder::getTextureHeight() {
    return 128;
}

int32_t OpenGlTextureHolder::attachToGraphics() {
    return 0;
}

void OpenGlTextureHolder::clearFromGraphics() {

}
