#pragma once
#include "TextureHolderInterface.h"

#include <GLES2/gl2.h>
#include <cstdint>
#include <memory>

#include <emscripten/val.h>

class ImageValTextureHolder : public TextureHolderInterface {
private:
    ImageValTextureHolder(emscripten::val img_);
public:
    static std::shared_ptr<ImageValTextureHolder> create(emscripten::val img_);
    virtual ~ImageValTextureHolder() = default;

    // Getter methods for image and texture dimensions
    virtual int32_t getImageWidth() override { return width; }
    virtual int32_t getImageHeight() override { return height; }
    virtual int32_t getTextureWidth() override { return width; }
    virtual int32_t getTextureHeight() override { return height; }

    // Attaches the texture to the graphics system
    virtual int32_t attachToGraphics() override;

    // Clears the texture from the graphics system
    virtual void clearFromGraphics() override;

public:
    // Reap garbage texture holders.
    // MUST be called _only_ on browser main thread.
    static void collectGarbage();

private:
    int32_t width;
    int32_t height;
    int32_t usageCounter;
    emscripten::val img;
    GLuint textureId;
};
