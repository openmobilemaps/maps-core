//
// Created by Christoph Maurhofer on 29.01.2021.
//

#include "ShaderFactoryOpenGl.h"
#include "AlphaShaderOpenGl.h"
#include "ColorShaderOpenGl.h"
#include "ColorLineShaderOpenGl.h"


std::shared_ptr<AlphaShaderInterface> ShaderFactoryOpenGl::createAlphaShader() {
    return std::make_shared<AlphaShaderOpenGl>();
}

std::shared_ptr<ColorLineShaderInterface> ShaderFactoryOpenGl::createColorLineShader() {
    return std::make_shared<ColorLineShaderOpenGl>();
}

std::shared_ptr<ColorShaderInterface> ShaderFactoryOpenGl::createColorShader() {
    return std::make_shared<ColorShaderOpenGl>();
}

