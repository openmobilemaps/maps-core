#pragma once
#include "TextureHolderInterface.h"

#include <GLES2/gl2.h>
#include <cstdint>
#include <memory>

#include <emscripten/val.h>

class ImageValTextureHolder : public TextureHolderInterface {
private:
    ImageValTextureHolder(emscripten::val img_, bool unpackPremultiplyAlpha_);
public:
    /**
     * Create an ImageValTextureHolder
     *
     * @pre must be called on main/graphics thread
     * @param img                     A "pixel source" object supported as source in gl.texImage2D
     * @param unpackPremultiplyAlpha  Multiply the alpha channel into the other
     *                                color channels when loading the image into a GL texture.
     *                                Default true.
     */
    static std::shared_ptr<ImageValTextureHolder> create(emscripten::val img, bool unpackPremultiplyAlpha = true);
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
    bool unpackPremultiplyAlpha;
};
