#pragma once

#include "PolygonLayerInterface.h"
#include "PolygonCompare.h"
#include "Polygon2dLayerObject.h"
#include <mutex>
#include <unordered_map>
#include <vector>


class PolygonLayer:
    public PolygonLayerInterface,
    public LayerInterface,
    public std::enable_shared_from_this<PolygonLayer>{
public:
    PolygonLayer();

    ~PolygonLayer() { };

// PolygonLayerInterface
    virtual void setPolygons(const std::vector<Polygon> & polygons) override;

    virtual std::vector<Polygon> getPolygons() override;

    virtual void remove(const Polygon & polygon) override;

    virtual void add(const Polygon & polygon) override;

    virtual void clear() override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

// LayerInterface
    virtual void update() override {};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> & mapInterface) override;

    virtual void onRemoved() override {};

    virtual void pause() override {};

    virtual void resume() override {};

    virtual void hide() override {};

    virtual void show() override {};
private:
    std::shared_ptr<MapInterface> mapInterface;

    std::recursive_mutex polygonsMutex;
    std::unordered_map<Polygon, std::shared_ptr<Polygon2dLayerObject>> polygons;

    void generateRenderPasses();
    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;
};
