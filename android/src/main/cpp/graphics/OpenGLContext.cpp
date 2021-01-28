#include "OpenGLContext.h"
#include "opengl_wrapper.h"

OpenGLContext::OpenGLContext() : programs(), texturePointers() {
}

int OpenGLContext::getProgram(std::string name) {
    if (programs.find(name) == programs.end()) {
        return 0;
    } else {
        return programs[name];
    }
}

void OpenGLContext::storeProgram(std::string name, int program) {
    programs[name] = program;
}

std::vector<unsigned int> &OpenGLContext::getTexturePointerArray(std::string name, int capacity) {
    if (texturePointers.find(name) == texturePointers.end()) {
        texturePointers[name] = std::vector<unsigned int>(capacity, 0);
    }

    return texturePointers[name];
}

void OpenGLContext::cleanAll() {
    for (std::map<std::string, int>::iterator it = programs.begin(); it != programs.end(); ++it) {
        glDeleteProgram(it->second);
    }

    programs.clear();

    for (std::map<std::string, std::vector<unsigned int>>::iterator it = texturePointers.begin();
         it != texturePointers.end(); ++it) {
        glDeleteTextures(GLsizei(it->second.size()), &it->second[0]);
    }
    texturePointers.clear();
}

void OpenGLContext::onSurfaceCreated() {
    cleanAll();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void OpenGLContext::setViewportSize(const ::Vec2I & size) {
    viewportSize = size;
    glViewport(0, 0, size.x, size.y);
}

::Vec2I OpenGLContext::getViewportSize() {
    return viewportSize;
}
