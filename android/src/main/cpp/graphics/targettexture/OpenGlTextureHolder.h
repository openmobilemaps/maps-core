/*
 * Copyright (c) 2022 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma  once

#include "TextureHolderInterface.h"
#include "opengl_wrapper.h"

class OpenGlTextureHolder : public TextureHolderInterface {
public:
    OpenGlTextureHolder();

    int32_t getImageWidth() override;

    int32_t getImageHeight() override;

    int32_t getTextureWidth() override;

    int32_t getTextureHeight() override;

    int32_t attachToGraphics() override;

    void clearFromGraphics() override;

private:
    GLuint renderedTexture = 0;
};