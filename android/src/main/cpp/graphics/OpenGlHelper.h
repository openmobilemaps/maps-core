//
// Created by Christoph Maurhofer on 20.02.2020.
//

#ifndef SWISSTOPO_OPENGLHELPER_H
#define SWISSTOPO_OPENGLHELPER_H

#include "logger/Logger.h"
#include "opengl_wrapper.h"

class OpenGlHelper {
public:
    static void checkGlError(std::string glOperation) {
        int error;
        while ((error = glGetError()) != GL_NO_ERROR) {
            LogError << "GL ERROR: " << glOperation << " " <<= error;
        }
    }
};


#endif //SWISSTOPO_OPENGLHELPER_H
