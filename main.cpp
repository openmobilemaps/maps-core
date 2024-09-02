#include "MapInterface.h"
#include "SceneInterface.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "CoordinateSystemFactory.h"
#include "Color.h"
#include "Vec2I.h"
#include "MapCamera2dInterface.h"
#include "MapCallbackInterface.h"
#include "ThreadPoolCallbacks.h"

#include "PolygonLayerInterface.h"
#include "PolygonInfo.h"
#include "LayerReadyState.h"

#include <iostream>
#include <memory>

#include <GL/osmesa.h>

struct TmpThreadPoolCallbacks : ThreadPoolCallbacks{
    ~TmpThreadPoolCallbacks() = default;
    std::string getCurrentThreadName() override { return ""; };
    void setCurrentThreadName(const std::string &name) override {}
    void setThreadPriority(TaskPriority priority) override {}
    void attachThread() override {}
    void detachThread() override {}
};

struct MapCallbackHandler : MapCallbackInterface {
  void invalidate() override {}
  void onMapResumed() override {}
};

int main() {
  const MapConfig mapConfig{CoordinateSystemFactory::getEpsg2056System()};
  auto csid = mapConfig.mapCoordinateSystem.identifier;
  const float pixelDensity = 1.0f; // ??
  std::shared_ptr<ThreadPoolCallbacks> threadCallbacks = std::make_shared<TmpThreadPoolCallbacks>();
  auto map = MapInterface::createWithOpenGl(mapConfig, pixelDensity, threadCallbacks);
  std::cout << map << std::endl;

  std::shared_ptr<MapCallbackInterface> mapCallbacks = std::make_shared<MapCallbackHandler>();
  map->setCallbackHandler(mapCallbacks);

  OSMesaContext ctx = OSMesaCreateContext(OSMESA_RGBA, nullptr);
  if (ctx == 0) {
    std::cerr << "Erro creating OSMesa context" << std::endl;
    return 3;
  }

  const size_t width = 400;
  const size_t height = 400;
  void* buf = malloc(width * height * 4);
  auto ok = OSMesaMakeCurrent(ctx, buf, GL_UNSIGNED_BYTE, width, height);
  if (!ok) {
    std::cerr << "Erro creating OSMesa context" << std::endl;
    return 3;
  }
  map->getRenderingContext()->onSurfaceCreated();
  map->setViewportSize(Vec2I(width, height));
  map->setBackgroundColor(Color{1.f, 1.f, 1.f, 1.0f});
  auto cam = MapCamera2dInterface::create(map, 1.0f);
  cam->moveToCenterPosition(Coord{csid, 0.5f, 0.5f, 0.f}, false);

  auto b = cam->getBounds();
  printf("[%f, %f, %f]  [%f, %f, %f]\n",
      b.topLeft.x, b.topLeft.y, b.topLeft.z,
      b.bottomRight.x, b.bottomRight.y, b.bottomRight.z
      );
  
  map->setCamera(cam);
  map->resume();
  
  auto polygonLayer = PolygonLayerInterface::create();
  polygonLayer->add(PolygonInfo{
      "Polygon",
      PolygonCoord{
        std::vector<Coord>{ 
          Coord{ csid, 0.f, 0.f, 0.f},
          Coord{ csid, 1.f, 0.f, 0.f},
          Coord{ csid, 1.f, 1.f, 0.f},
          Coord{ csid, 0.f, 1.f, 0.f},
          Coord{ csid, 0.f, 0.f, 0.f}, },
        std::vector<std::vector<Coord>>{},
      },
      Color(0.0f, 0.0f, 0.0f, 1.0f),
      Color(0.0f, 0.0f, 0.0f, 1.0f),
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

  glFinish();
  printf("%zx\n", *(size_t*)buf);

  FILE *f = fopen("frame.pam", "w");
  fprintf(f, "P7\nWIDTH %zu\nHEIGHT %zu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", width, height);
  fwrite(buf, width * height * 4, 1, f);
  fclose(f);

  //auto scheduler = std::make_shared<ThreadPoolSchedulerImpl>(std::make_shared<AndroidSchedulerCallback>());
//auto map = std::make_shared<MapScene>(SceneInterface::createWithOpenGl(), mapConfig, scheduler, pixelDensity);
  //std::cout << map << std::endl;
}
