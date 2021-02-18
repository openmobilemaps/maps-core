//
// Created by Christoph Maurhofer on 29.01.2021.
//

#ifndef MAPSDK_SHADERFACTORYOPENGL_H
#define MAPSDK_SHADERFACTORYOPENGL_H

#include "ShaderFactoryInterface.h"

class ShaderFactoryOpenGl : public ShaderFactoryInterface {

    virtual std::shared_ptr<AlphaShaderInterface> createAlphaShader() override;

    virtual std::shared_ptr<ColorLineShaderInterface> createColorLineShader() override;

    virtual std::shared_ptr<ColorShaderInterface> createColorShader() override;
};

#endif // MAPSDK_SHADERFACTORYOPENGL_H
