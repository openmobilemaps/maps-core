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

#include "RenderTargetTexture.h"
#include "OpenGlTextureHolder.h"
#include "Vec2I.h"

class OpenGlRenderTargetTexture : public RenderTargetTexture {
public:
    OpenGlRenderTargetTexture();

    void setViewPortSize(const ::Vec2I &size);

    std::shared_ptr<::TextureHolderInterface> textureHolder() override;


private:
   std::shared_ptr<::OpenGlTextureHolder> holder;
};
