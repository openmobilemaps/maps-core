#include <algorithm>
#include <cmath>
#define GL_GLEXT_PROTOTYPES 1

#include "Color.h"
#include "CoordinateSystemFactory.h"
#include "LayerReadyState.h"
#include "MapCallbackInterface.h"
#include "MapCameraInterface.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "PolygonInfo.h"
#include "PolygonLayerInterface.h"
#include "ThreadPoolScheduler.h"
#include "Vec2I.h"

#include <iostream>
#include <memory>
#include <string.h>

#include <GL/osmesa.h>

#define WITH_MSAA 1

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

struct NopMapCallback : MapCallbackInterface {
    void invalidate() override {}
    void onMapResumed() override {}
};

static void awaitReadyToRenderOffscreen(std::shared_ptr<MapInterface> map);

int main() {
    OSMesaContext ctx = initOSMesa();
    if (ctx == nullptr) {
        std::cerr << "Error creating OSMesa context" << std::endl;
        return 3;
    }

    const size_t width = 300;
    const size_t height = 270;
    void *buf = malloc(width * height * 4);
    auto ok = OSMesaMakeCurrent(ctx, buf, GL_UNSIGNED_BYTE, width, height);
    if (!ok) {
        std::cerr << "Error OSMesaMakeCurrent" << std::endl;
        return 3;
    }

#if WITH_MSAA
    MSAAState msaaState = initMSAA(4, width, height);
#endif

    const MapConfig mapConfig{CoordinateSystemFactory::getEpsg3857System()};
    const float pixelDensity = 90.0f; // ??
    auto map = MapInterface::createWithOpenGl(mapConfig, ThreadPoolScheduler::create(), pixelDensity, false);

    map->setCallbackHandler(std::make_shared<NopMapCallback>());

    map->getRenderingContext()->onSurfaceCreated();
    map->setViewportSize(Vec2I(width, height));
    map->setBackgroundColor(Color{0.9f, 0.9f, 0.9f, 1.0f});
    // map->setCamera(MapCamera2dInterface::create(map, 1.0f)); // BUG! must be _before_ setViewportSize
    map->resume();

    {
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

    FILE *f = fopen("frame.pam", "w");
    fprintf(f, "P7\nWIDTH %zu\nHEIGHT %zu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", width, height);
    fwrite(buf, width * height * 4, 1, f);
    fclose(f);

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
