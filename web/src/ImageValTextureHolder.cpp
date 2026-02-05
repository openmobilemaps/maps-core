#include "ImageValTextureHolder.h"

#include <emscripten.h>
#ifndef NDEBUG
#include <emscripten/threading.h>
#endif

#include <list>

EM_JS(void, gl_tex_image_2d, (emscripten::EM_VAL img_handle), {
    var img = Emval.toValue(img_handle);
    var gl = GLctx;
    return gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, img.width, img.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, img);
});

ImageValTextureHolder::ImageValTextureHolder(emscripten::val img_)
  : usageCounter(0)
{
    width = img_["width"].as<uint32_t>();
    height = img_["height"].as<uint32_t>();
    img = img_;
}

int32_t ImageValTextureHolder::attachToGraphics() {
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
void ImageValTextureHolder::clearFromGraphics() {
    if (--usageCounter == 0 && textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
}

/**
 * Explicitly keep alive emscripten::val image values on garbagePile, to ensure
 * destruction on the main thread that created the values.
 */
static std::list<std::shared_ptr<ImageValTextureHolder>> garbagePile;

std::shared_ptr<ImageValTextureHolder> ImageValTextureHolder::create(emscripten::val img) {
    auto textureHolder = std::shared_ptr<ImageValTextureHolder>(new ImageValTextureHolder(img));
    garbagePile.push_back(textureHolder);
    return textureHolder;
}

void ImageValTextureHolder::collectGarbage() {
    // no lock, garbagePile must _only_ be accessed on the main thread.
    assert(pthread_self() == emscripten_main_runtime_thread_id());
    for (auto it = garbagePile.begin(); it != garbagePile.end();) {
        std::weak_ptr<ImageValTextureHolder> weak = *it;
        *it = nullptr;
        *it = weak.lock();
        if (*it == nullptr) {
            it = garbagePile.erase(it);
        } else {
            ++it;
        }
    }
}
