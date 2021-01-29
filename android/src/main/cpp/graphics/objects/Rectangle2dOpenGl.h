//
// Created by Christoph Maurhofer on 28.01.2021.
//

#ifndef MAPSDK_RECTANGLE2DOPENGL_H
#define MAPSDK_RECTANGLE2DOPENGL_H


#include <vector>
#include "ShaderProgramInterface.h"
#include "GraphicsObjectInterface.h"
#include "Rectangle2dInterface.h"
#include "opengl_wrapper.h"
#include "OpenGLContext.h"

class Rectangle2dOpenGl
        : public GraphicsObjectInterface,
          public Rectangle2dInterface,
          public std::enable_shared_from_this<GraphicsObjectInterface> {
public:
    Rectangle2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~Rectangle2dOpenGl() {};

    virtual bool isReady();

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void clear();

    virtual void
    render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass, int64_t mvpMatrix);

    virtual void setFrame(const ::RectF &frame, const ::RectF &textureCoordinates);

    virtual void loadTexture(const std::shared_ptr<TextureHolderInterface> &textureHolder);

    virtual void removeTexture();

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject();

protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(std::shared_ptr<OpenGLContext> &openGLContext, int mProgram);

    std::shared_ptr<ShaderProgramInterface> shaderProgram;

    std::vector<GLfloat> vertexBuffer;
    std::vector<GLfloat> textureBuffer;
    std::vector<GLubyte> indexBuffer;
    std::vector<GLuint> texturePointer;
    bool textureLoaded = false;

    RectF frame = RectF(0.0f, 0.0f, 0.0f, 0.0f);
    RectF textureCoordinates = RectF(0.0f, 0.0f, 0.0f, 0.0f);
    float factorHeight = 1.0f;
    float factorWidth = 1.0f;

    bool ready = false;
};


#endif //MAPSDK_RECTANGLE2DOPENGL_H
