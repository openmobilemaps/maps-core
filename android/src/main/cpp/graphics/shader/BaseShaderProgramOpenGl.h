//
// Created by Christoph Maurhofer on 20.02.2020.
//

#ifndef SWISSTOPO_BASESHADERPROGRAMOPENGL_H
#define SWISSTOPO_BASESHADERPROGRAMOPENGL_H

#define UBRendererShaderCode(...) #__VA_ARGS__

#include "ShaderProgramInterface.h"
#include "../logger/Logger.h"
#include "opengl_wrapper.h"

class BaseShaderProgramOpenGl {
protected:
    int loadShader(int type, std::string shaderCode);

    void checkGlProgramLinking(GLuint program);

    virtual std::string getVertexShader();

    virtual std::string getFragmentShader();
};


#endif //SWISSTOPO_BASESHADERPROGRAMOPENGL_H
