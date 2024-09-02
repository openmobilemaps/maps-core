#include "Color.h"
#include "CoordinateSystemFactory.h"
#include "MapCallbackInterface.h"
#include "MapCamera2dInterface.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "SceneInterface.h"
#include "ThreadPoolCallbacks.h"
#include "Vec2I.h"

#include "LayerReadyState.h"
#include "PolygonInfo.h"
#include "PolygonLayerInterface.h"

#include <iostream>
#include <limits>
#include <memory>

#include <GL/osmesa.h>

struct MapCallbackHandler : MapCallbackInterface {
    void invalidate() override {}
    void onMapResumed() override {}
};

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
#define glCheckError() _x_glCheckError_(__FILE__, __LINE__)

static void printRect(const char *h, const RectCoord &b) {
    printf("%s:\t[%f, %f, %f]  [%f, %f, %f]\n", h, b.topLeft.x, b.topLeft.y, b.topLeft.z, b.bottomRight.x, b.bottomRight.y,
           b.bottomRight.z);
}

int main() {
#if OSMESA_MAJOR_VERSION > 11 || (OSMESA_MAJOR_VERSION == 11 && OSMESA_MINOR_VERSION >= 2)
    int osmesa_attribs[] = {
        OSMESA_FORMAT,
        OSMESA_RGBA,
        OSMESA_PROFILE,
        OSMESA_COMPAT_PROFILE,
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
    if (ctx == 0) {
        std::cerr << "Error creating OSMesa context" << std::endl;
        return 3;
    }

    const size_t width = 400;
    const size_t height = 400;
    void *buf = malloc(width * height * 4);
    auto ok = OSMesaMakeCurrent(ctx, buf, GL_UNSIGNED_BYTE, width, height);
    if (!ok) {
        std::cerr << "Error OSMesaMakeCurrent" << std::endl;
        return 3;
    }

    const MapConfig mapConfig{CoordinateSystemFactory::getEpsg2056System()};
    auto csid = mapConfig.mapCoordinateSystem.identifier;
    const float pixelDensity = 1.0f; // ??
    auto map = MapInterface::createWithOpenGl(mapConfig, pixelDensity);
    std::cout << map << std::endl;

    std::shared_ptr<MapCallbackInterface> mapCallbacks = std::make_shared<MapCallbackHandler>();
    map->setCallbackHandler(mapCallbacks);

    printf("Vendor graphic card: %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version GL: %s\n", glGetString(GL_VERSION));
    printf("Version GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    map->getRenderingContext()->onSurfaceCreated();
    map->setViewportSize(Vec2I(width, height));
    map->setBackgroundColor(Color{0.3f, 0.3f, 0.3f, 1.0f});
    auto cam = MapCamera2dInterface::create(map, 1.0f);
    map->setCamera(cam);
    map->resume();

    cam->setPaddingTop(1000.f);
    cam->setPaddingBottom(1000.f);
    cam->setPaddingLeft(1000.f);
    cam->setPaddingRight(1000.f);
    printRect("bounds", cam->getBounds());
    printRect("visibl", cam->getVisibleRect());
    printRect("vispad", cam->getPaddingAdjustedVisibleRect());

    auto visible = cam->getPaddingAdjustedVisibleRect();
    float x0 = visible.topLeft.x;
    float y0 = visible.topLeft.y;
    float x1 = visible.bottomRight.x;
    float y1 = visible.bottomRight.y;
    printf("[%f, %f, %f]  [%f, %f, %f]\n", x0, y0, 0.f, x1, y1, 0.f);

    auto polygonLayer = PolygonLayerInterface::create();
    polygonLayer->addAll(std::vector<PolygonInfo>{
        PolygonInfo{
            "poly1",
            PolygonCoord{
                std::vector<Coord>{
                    Coord{csid, x0, y0, 0.f},
                    Coord{csid, x1, y0, 0.f},
                    Coord{csid, x1, y1, 0.f},
                    Coord{csid, x0, y1, 0.f},
                    Coord{csid, x0, y0, 0.f},
                },
                std::vector<std::vector<Coord>>{},
            },
            Color(0.0f, 1.0f, 0.0f, 0.2f),
            Color(0.0f, 1.0f, 0.0f, 0.2f),
        },
        PolygonInfo{
            "poly2",
            PolygonCoord{
                std::vector<Coord>{
                    Coord{csid, x0, y0, 0.f},
                    Coord{csid, x1, y0, 0.f},
                    Coord{csid, x1, y1, 0.f},
                },
                std::vector<std::vector<Coord>>{},
            },
            Color(1.0f, 1.0f, 0.0f, 1.0f),
            Color(1.0f, 1.0f, 0.0f, 1.0f),
        },
        PolygonInfo{
            "poly3",
            PolygonCoord{
                std::vector<Coord>{
                    Coord{csid, x0, y0, 0.f},
                    Coord{csid, x1, y1, 0.f},
                    Coord{csid, x1, y0, 0.f},
                },
                std::vector<std::vector<Coord>>{},
            },
            Color(0.0f, 1.0f, 1.0f, 1.0f),
            Color(0.0f, 1.0f, 1.0f, 1.0f),
        },
    });
    auto player = polygonLayer->asLayerInterface();
    map->addLayer(player);

    auto state = player->isReadyToRenderOffscreen();
    while (state != LayerReadyState::READY) {
        printf("not ready\n");
        map->invalidate();
        state = player->isReadyToRenderOffscreen();
    }
    map->drawFrame();

    glCheckError();
    glFinish();
    glCheckError();
    printf("%zx\n", *(size_t *)buf);

    FILE *f = fopen("frame.pam", "w");
    fprintf(f, "P7\nWIDTH %zu\nHEIGHT %zu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", width, height);
    fwrite(buf, width * height * 4, 1, f);
    fclose(f);
}
