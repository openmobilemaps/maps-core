#pragma once

#include "Polygon2dLayerObject.h"
#include "PolygonCompare.h"
#include "PolygonLayerInterface.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PolygonLayer : public PolygonLayerInterface, public LayerInterface, public std::enable_shared_from_this<PolygonLayer> {
  public:
    PolygonLayer();

    ~PolygonLayer(){};

    // PolygonLayerInterface
    virtual void setPolygons(const std::vector<PolygonInfo> &polygons) override;

    virtual std::vector<PolygonInfo> getPolygons() override;

    virtual void remove(const PolygonInfo &polygon) override;

    virtual void add(const PolygonInfo &polygon) override;

    virtual void clear() override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    // LayerInterface
    virtual void update() override{};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override{};

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

  private:
    std::shared_ptr<MapInterface> mapInterface;

    std::recursive_mutex polygonsMutex;
    std::unordered_map<PolygonInfo, std::shared_ptr<Polygon2dLayerObject>> polygons;

    void generateRenderPasses();
    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;

    std::recursive_mutex addingQueueMutex;
    std::unordered_set<PolygonInfo> addingQueue;

    std::atomic<bool> isHidden;
};
