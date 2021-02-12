#include "OpenGlContext.h"
#include "opengl_wrapper.h"

OpenGlContext::OpenGlContext() : programs(), texturePointers() {
}

int OpenGlContext::getProgram(std::string name) {
    if (programs.find(name) == programs.end()) {
        return 0;
    } else {
        return programs[name];
    }
}

void OpenGlContext::storeProgram(std::string name, int program) {
    programs[name] = program;
}

std::vector<unsigned int> &OpenGlContext::getTexturePointerArray(std::string name, int capacity) {
    if (texturePointers.find(name) == texturePointers.end()) {
        texturePointers[name] = std::vector<unsigned int>(capacity, 0);
    }

    return texturePointers[name];
}

void OpenGlContext::cleanAll() {
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

void OpenGlContext::onSurfaceCreated() {
    cleanAll();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void OpenGlContext::setViewportSize(const ::Vec2I &size) {
    viewportSize = size;
    glViewport(0, 0, size.x, size.y);
}

::Vec2I OpenGlContext::getViewportSize() {
    return viewportSize;
}

void OpenGlContext::setBackgroundColor(const Color &color) {
    backgroundColor = color;
}

void OpenGlContext::setupDrawFrame() {
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
