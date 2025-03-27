#include "Font.h"
#define GL_GLEXT_PROTOTYPES 1
#include "Color.h"
#include "CoordinateSystemFactory.h"
#include "CoordinateSystemIdentifiers.h"
#include "DataLoaderResult.h"
#include "LayerReadyState.h"
#include "LoaderInterface.h"
#include "MapCallbackInterface.h"
#include "MapCameraInterface.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "PolygonInfo.h"
#include "PolygonLayerInterface.h"
#include "RectCoord.h"
#include "TaskInterface.h"
#include "TextureLoaderResult.h"
#include "ThreadPoolScheduler.h"
#include "Tiled2dMapVectorLayerInterface.h"
#include "Vec2I.h"
#include "FontLoaderResult.h"

#include <GL/osmesa.h>

#include "lodepng.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#define WITH_MSAA 0

#define glCheckError() _x_glCheckError_(__FILE__, __LINE__)
static GLenum _x_glCheckError_(const char *file, int line);

static OSMesaContext initOSMesa();

// State for multi sample anti-aliasing.
struct MSAAState {
    GLuint fbo;    // frame buffer
    GLuint rbo[2]; // color, depth&stencil render buffers
};
static MSAAState initMSAA(int numSamples, size_t width, size_t height);
static void readPixelsMSAA(MSAAState s, char *buf, size_t width, size_t height);

static void printRect(const char *h, const RectCoord &b);
static void addPolygonLayerUB(std::shared_ptr<MapInterface> map, RectCoord rect);
static void addStyleJsonLayer(std::shared_ptr<MapInterface> map, std::string url);

struct NopMapCallback : MapCallbackInterface {
    void invalidate() override {}
    void onMapResumed() override {}
};

static void awaitReadyToRenderOffscreen(std::shared_ptr<MapInterface> map);

class ImmediateScheduler : public SchedulerInterface {
    virtual void addTask(const /*not-null*/ std::shared_ptr<TaskInterface> &task) override { task->run(); }

    virtual void addTasks(const std::vector</*not-null*/ std::shared_ptr<TaskInterface>> &tasks) override {
        for (auto &task : tasks) {
            task->run();
        }
    }

    virtual void removeTask(const std::string &id) override {}

    virtual void clear() override {}

    virtual void pause() override {}

    virtual void resume() override {}

    virtual void destroy() override {}

    virtual bool hasSeparateGraphicsInvocation() override { return false; }

    /** Execute added graphics tasks. Returns true, if there are unprocessed tasks in the queue after the execution. */
    virtual bool runGraphicsTasks() override { return false; }

    virtual void
    setSchedulerGraphicsTaskCallbacks(const /*not-null*/ std::shared_ptr<SchedulerGraphicsTaskCallbacks> &callbacks) override {}
};

int main() {
    OSMesaContext ctx = initOSMesa();
    if (ctx == nullptr) {
        std::cerr << "Error creating OSMesa context" << std::endl;
        return 3;
    }

    const size_t width = 1000;
    const size_t height = 1000;
    void *buf = malloc(width * height * 4);
    auto ok = OSMesaMakeCurrent(ctx, buf, GL_UNSIGNED_BYTE, width, height);
    if (!ok) {
        std::cerr << "Error OSMesaMakeCurrent" << std::endl;
        return 3;
    }

    printf("Vendor graphic card: %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version GL: %s\n", glGetString(GL_VERSION));
    printf("Version GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

#if WITH_MSAA
    MSAAState msaaState = initMSAA(4, width, height);
#endif

    const MapConfig mapConfig{CoordinateSystemFactory::getEpsg3857System()};
    const float pixelDensity = 90.0f; // ??
    auto scheduler = ThreadPoolScheduler::create();
    // auto scheduler = std::make_shared<ImmediateScheduler>();
    auto map = MapInterface::createWithOpenGl(mapConfig, scheduler, pixelDensity, false);

    map->setCallbackHandler(std::make_shared<NopMapCallback>());

    map->getRenderingContext()->onSurfaceCreated();
    map->setViewportSize(Vec2I(width, height));
    map->setBackgroundColor(Color{0.9f, 0.9f, 0.9f, 1.0f});
    // map->setCamera(MapCamera2dInterface::create(map, 1.0f)); // BUG! must be _before_ setViewportSize
    map->resume();


    if (true) {
        auto cam = map->getCamera();
        cam->moveToBoundingBox(
            RectCoord{
                Coord(CoordinateSystemIdentifiers::EPSG4326(), 5.9, 47.9, 0.0),
                Coord(CoordinateSystemIdentifiers::EPSG4326(), 10.5, 45.8, 0.0),
            },
            0.f, false, std::nullopt, std::nullopt);
        cam->setZoom(cam->getZoom() / 4.f, false);
        printRect("bounds", cam->getBounds());
        printRect("visibl", cam->getVisibleRect());
        printRect("vispad", cam->getPaddingAdjustedVisibleRect());

        std::ifstream ifsJson("data/basemap-style-notext.json");
        std::string jsonData((std::istreambuf_iterator<char>(ifsJson)), (std::istreambuf_iterator<char>()));

        addStyleJsonLayer(map, jsonData);
    }
    
    if (false) {
        auto cam = map->getCamera();
        cam->setPaddingTop(10.f);
        cam->setPaddingBottom(10.f);
        cam->setPaddingLeft(10.f);
        cam->setPaddingRight(10.f);
        printRect("bounds", cam->getBounds());
        printRect("visibl", cam->getVisibleRect());
        printRect("vispad", cam->getPaddingAdjustedVisibleRect());

        auto visible = cam->getPaddingAdjustedVisibleRect();
        addPolygonLayerUB(map, visible);
    }

    awaitReadyToRenderOffscreen(map);
    map->prepare();
    map->drawFrame();
    glCheckError();
    glFinish();
    glCheckError();

    map->destroy();
    map = nullptr;

#if WITH_MSAA
    readPixelsMSAA(msaaState, (char *)buf, width, height);
#endif

#if 1
    FILE *f = fopen("frame.pam", "w");
    fprintf(f, "P7\nWIDTH %zu\nHEIGHT %zu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", width, height);
    for (int i = height - 1; i >= 0; i--) {
        size_t rowWidth = width * 4;
        fwrite((uint8_t *)buf + i * rowWidth, rowWidth, 1, f);
    }
    fclose(f);
#else
    auto bufFlipped = std::make_unique<uint8_t[]>(width*height*4);
    for (size_t i = 0; i < height; i++) {
        size_t rowWidth = width * 4;
        memcpy(bufFlipped.get() + (height - i - 1) * rowWidth, (uint8_t*)buf + i*rowWidth, rowWidth);
    }
    lodepng::encode("frame.png", bufFlipped.get(), width, height);
    printf("written\n");
#endif

    OSMesaDestroyContext(ctx);
    free(buf);
}

static GLenum _x_glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            error = "STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << ":" << line << std::endl;
    }
    return errorCode;
}

static void printRect(const char *header, const RectCoord &b) {
    printf("%s:\t[%f, %f, %f]  [%f, %f, %f]\n",
           header,                                             //
           b.topLeft.x, b.topLeft.y, b.topLeft.z,              //
           b.bottomRight.x, b.bottomRight.y, b.bottomRight.z); //
}

static OSMesaContext initOSMesa() {
#if OSMESA_MAJOR_VERSION > 11 || (OSMESA_MAJOR_VERSION == 11 && OSMESA_MINOR_VERSION >= 2)
    int osmesa_attribs[] = {
        OSMESA_FORMAT,
        OSMESA_RGBA,
        OSMESA_PROFILE,
        OSMESA_COMPAT_PROFILE,
        OSMESA_STENCIL_BITS,
        8,
        OSMESA_DEPTH_BITS,
        16,
        OSMESA_ACCUM_BITS,
        16,
        OSMESA_CONTEXT_MAJOR_VERSION,
        3,
        OSMESA_CONTEXT_MINOR_VERSION,
        2,
        0,
    };
    OSMesaContext ctx = OSMesaCreateContextAttribs(osmesa_attribs, nullptr);
#else
    // Just hope
    OSMesaContext ctx = OSMesaCreateContext(OSMESA_RGBA, nullptr);
#endif
    return ctx;
}

static MSAAState initMSAA(int numSamples, size_t width, size_t height) {
    GLuint fbo = -1;
    GLuint rbo[2] = {(GLuint)-1, (GLuint)-1};

    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(2, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA, width, height);
    glCheckError();
    glBindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
    glCheckError();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);

    auto fbStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    switch (fbStatus) {
    case GL_FRAMEBUFFER_COMPLETE:
        break;
    case GL_FRAMEBUFFER_UNDEFINED:
        printf("GL_FRAMEBUFFER_UNDEFINED is returned if the specified framebuffer is the default read or draw framebuffer, but the "
               "default framebuffer does not exist.\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        printf("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer "
               "incomplete.");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        printf("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one image "
               "attached to it.");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        printf("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE "
               "for any color attachment point(s) named by GL_DRAW_BUFFERi.");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        printf("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is not GL_NONE and the value of "
               "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        printf("GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal formats of the attached images violates an "
               "implementation-dependent set of restrictions.");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        printf("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all "
               "attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the "
               "attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the "
               "value of GL_TEXTURE_SAMPLES.");
        break;
    }
    glCheckError();
    return MSAAState{.fbo = fbo, .rbo = {rbo[0], rbo[1]}};
}

static void readPixelsMSAA(MSAAState s, char *buf, size_t width, size_t height) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s.fbo);

    glFinish();
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glCheckError();
}

class TextureHolder : public TextureHolderInterface {

  public:
    TextureHolder(uint32_t width, uint32_t height, std::vector<uint8_t> dataARGB)
        : width(width)
        , height(height)
        , dataARGB(std::move(dataARGB)) {}

    virtual int32_t getImageWidth() override { return width; }
    virtual int32_t getImageHeight() override { return height; }
    virtual int32_t getTextureWidth() override { return width; }
    virtual int32_t getTextureHeight() override { return height; }
    virtual int32_t attachToGraphics() override {
        GLuint textureId;
        glCheckError();
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glCheckError();

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_BGRA, GL_UNSIGNED_BYTE, &dataARGB[0]);
        glCheckError();

        return textureId;
    }
    virtual void clearFromGraphics() override {}

  private:
    int32_t width;
    int32_t height;
    std::vector<uint8_t> dataARGB;
};

class DummyLoader : public LoaderInterface
                  , public FontLoaderInterface
{
  public:
    DummyLoader(std::string directory)
        : directory(directory) {}

    virtual TextureLoaderResult loadTexture(const std::string &url, const std::optional<std::string> &etag) override {
        std::cout << "loadTexture " << url << std::endl;
        auto data = readPng(urlToFilename(url));
        return TextureLoaderResult{data, std::nullopt, LoaderStatus::OK, std::nullopt};
    }
    virtual DataLoaderResult loadData(const std::string &url, const std::optional<std::string> &etag) override {
        std::cout << "loadData " << url << std::endl;
        auto data = readData(urlToFilename(url));
        if(data) {
          return DataLoaderResult{djinni::DataRef{std::move(*data)}, std::nullopt, LoaderStatus::OK, std::nullopt};
        } else {
          std::cout << "loadData " << url << " FAIL" << std::endl;
          return DataLoaderResult{std::nullopt, std::nullopt, LoaderStatus::ERROR_404, std::nullopt};
        }
    }

    virtual ::djinni::Future<TextureLoaderResult> loadTextureAsync(const std::string &url,
                                                                   const std::optional<std::string> &etag) override {
        djinni::Promise<TextureLoaderResult> promise;
        promise.setValue(loadTexture(url, etag));
        return promise.getFuture();
    }

    virtual ::djinni::Future<DataLoaderResult> loadDataAsync(const std::string &url,
                                                             const std::optional<std::string> &etag) override {
        djinni::Promise<DataLoaderResult> promise;
        promise.setValue(loadData(url, etag));
        return promise.getFuture();
    }
    virtual void cancel(const std::string &url) override {}

    virtual FontLoaderResult loadFont(const Font & font) override {
      return FontLoaderResult{
        nullptr,
        std::nullopt,
        LoaderStatus::OK,
      };
    }

  private:
    std::string urlToFilename(std::string url) {
        if (url.starts_with("https://")) {
            url = url.substr(strlen("https://"));
        } else if (url.starts_with("http://")) {
            url = url.substr(strlen("http://"));
        }
        std::replace(url.begin(), url.end(), '/', '_');
        return directory + url;
    }

    std::optional<std::vector<uint8_t>> readData(std::string filename) {
        std::ifstream file(filename, std::ios::binary);
        std::vector<uint8_t> fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if(file.fail()) {
          return std::nullopt;
        }
        return fileContents;
    }

    std::shared_ptr<TextureHolderInterface> readPng(std::string filename) {
        std::vector<uint8_t> image; // the raw pixels
        unsigned width, height;
        unsigned error = lodepng::decode(image, width, height, filename);
        if (error)
            std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        return std::make_shared<TextureHolder>(width, height, image);
    }

  private:
    std::string directory;
};

static void addStyleJsonLayer(std::shared_ptr<MapInterface> map, std::string styleJsonData) {
    std::vector<std::shared_ptr<LoaderInterface>> loaders = {std::make_shared<DummyLoader>("data/")};
    auto layer = Tiled2dMapVectorLayerInterface::createExplicitly("name", styleJsonData, true, loaders, nullptr, nullptr,
                                                                  std::nullopt, nullptr, std::nullopt);

    map->addLayer(layer->asLayerInterface());
}

// Add a demo polygon layer with two polygons inside the given rectangle.
static void addPolygonLayerUB(std::shared_ptr<MapInterface> map, RectCoord rect) {
    const auto csid = rect.topLeft.systemIdentifier;
    const float x0 = rect.topLeft.x;
    const float y0 = rect.topLeft.y;
    const float x1 = rect.bottomRight.x;
    const float y1 = rect.bottomRight.y;

    auto coord = [=](float x, float y) {
        return Coord{
            csid,                 //
            std::lerp(x0, x1, x), //
            std::lerp(y0, y1, y), //
            0.f                   //
        };
    };

    auto polygonLayer = PolygonLayerInterface::create();
    polygonLayer->addAll(std::vector<PolygonInfo>{
        PolygonInfo{
            "raute",
            PolygonCoord{
                std::vector<Coord>{
                    coord(0.0332951, 0.569277),
                    coord(0.445824, 0.874362),
                    coord(0.518277, 0.753463),
                    coord(0.402097, 0.689869),
                    coord(0.380351, 0.352679),
                    coord(0.508078, 0.259091),
                    coord(0.307968, 0.111111),
                },
                std::vector<std::vector<Coord>>{},
            },
            Color(0.18f, 0.541f, 0.902f, 1.f),
            Color(0.f, 0.f, 0.f, 0.f),
        },
        PolygonInfo{
            "hexagon",
            PolygonCoord{
                std::vector<Coord>{
                    coord(0.507147, 0.258516),
                    coord(0.719898, 0.415755),
                    coord(0.517378, 0.753342),
                    coord(0.677265, 0.840793),
                    coord(0.931705, 0.654475),
                    coord(0.90995, 0.317027),
                    coord(0.633686, 0.165881),
                },
                std::vector<std::vector<Coord>>{},
            },
            Color(0.106f, 0.227f, 0.349f, 1.f),
            Color(0.f, 0.f, 0.f, 0.f),
        },
    });
    auto player = polygonLayer->asLayerInterface();
    map->addLayer(player);
}

static void awaitReadyToRenderOffscreen(std::shared_ptr<MapInterface> map) {

    bool unready = true;
    while (unready) {
        while (map->getScheduler()->runGraphicsTasks()) {
        };

        unready = std::ranges::any_of(map->getLayers(), [](std::shared_ptr<LayerInterface> &layer) -> bool {
            return layer->isReadyToRenderOffscreen() == LayerReadyState::NOT_READY;
        });
    }

    // check for error states
    for (auto layer : map->getLayers()) {
        const auto layerState = layer->isReadyToRenderOffscreen();
        if (layerState != LayerReadyState::READY) {
            printf("layer in error state: %s\n", toString(layerState));
        }
    }
}
