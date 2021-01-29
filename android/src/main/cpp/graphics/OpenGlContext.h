#ifndef MAPSDK_OPENGLCONTEXT_H
#define MAPSDK_OPENGLCONTEXT_H

#include <vector>
#include <string>
#include <map>
#include "RenderingContextInterface.h"

class OpenGlContext : public RenderingContextInterface, std::enable_shared_from_this<OpenGlContext> {
public:
    OpenGlContext();

    int getProgram(std::string name);

    void storeProgram(std::string name, int program);

    std::vector<unsigned int> &getTexturePointerArray(std::string name, int capacity);

    void cleanAll();

    virtual void onSurfaceCreated();

    virtual void setViewportSize(const ::Vec2I & size);

    virtual ::Vec2I getViewportSize();

    virtual void setupDrawFrame();

protected:
    std::map<std::string, int> programs;
    std::map<std::string, std::vector<unsigned int>> texturePointers;

    Vec2I viewportSize = Vec2I(0, 0);
};

#endif // MAPSDK_OPENGLCONTEXT_H
