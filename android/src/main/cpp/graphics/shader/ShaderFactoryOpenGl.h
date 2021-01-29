//
// Created by Christoph Maurhofer on 29.01.2021.
//

#ifndef MAPSDK_SHADERFACTORYOPENGL_H
#define MAPSDK_SHADERFACTORYOPENGL_H

#include "ShaderFactoryInterface.h"

class ShaderFactoryOpenGl : public ShaderFactoryInterface {

    virtual std::shared_ptr<AlphaShaderInterface> createAlphaShader();

    virtual std::shared_ptr<ColorLineShaderInterface> createColorLineShader();

    virtual std::shared_ptr<ColorShaderInterface> createColorShader();

};


#endif //MAPSDK_SHADERFACTORYOPENGL_H
