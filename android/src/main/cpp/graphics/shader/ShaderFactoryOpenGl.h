/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSDK_SHADERFACTORYOPENGL_H
#define MAPSDK_SHADERFACTORYOPENGL_H

#include "ShaderFactoryInterface.h"

class ShaderFactoryOpenGl : public ShaderFactoryInterface {

    virtual std::shared_ptr<AlphaShaderInterface> createAlphaShader() override;

    virtual std::shared_ptr<ColorLineShaderInterface> createColorLineShader() override;

    virtual std::shared_ptr<ColorShaderInterface> createColorShader() override;
};

#endif // MAPSDK_SHADERFACTORYOPENGL_H
