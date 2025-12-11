#include "TextureHolderInterface.h"
#include <GLES2/gl2.h>

EM_JS(void, gl_tex_image_2d, (em::EM_VAL img_handle), {
    var img = Emval.toValue(img_handle);
    var gl = GLctx;
    return gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, img.width, img.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, img);
})

class ImageValTextureHolder : public TextureHolderInterface {
  public:
    // Constructor
    ImageValTextureHolder(em::val img_)
        : usageCounter(0) {
        width = img_["width"].as<uint32_t>();
        height = img_["height"].as<uint32_t>();
        img = img_;
    }
    virtual ~ImageValTextureHolder() override {}

    // Getter methods for image and texture dimensions
    virtual int32_t getImageWidth() override { return width; }
    virtual int32_t getImageHeight() override { return height; }
    virtual int32_t getTextureWidth() override { return width; }
    virtual int32_t getTextureHeight() override { return height; }

    // Attaches the texture to the graphics system
    virtual int32_t attachToGraphics() override {
        if (usageCounter++ == 0) {
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl_tex_image_2d(img.as_handle());
        }
        return textureId;
    }

    // Clears the texture from the graphics system
    virtual void clearFromGraphics() override {
        if (--usageCounter == 0 && textureId != 0) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
    }

  private:
    int32_t width;
    int32_t height;
    int32_t usageCounter;
    em::val img;
    GLuint textureId;
};