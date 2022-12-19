/*
 * Copyright (c) 2022 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "OpenGlRenderTargetTexture.h"

OpenGlRenderTargetTexture::OpenGlRenderTargetTexture() :
    holder(std::make_shared<OpenGlTextureHolder>()) {}

std::shared_ptr<::TextureHolderInterface> OpenGlRenderTargetTexture::textureHolder() {
    return holder;
}

void OpenGlRenderTargetTexture::setViewPortSize(const Vec2I &size) {

}
