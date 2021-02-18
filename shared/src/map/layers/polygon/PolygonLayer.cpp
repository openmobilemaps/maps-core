#include "PolygonLayer.h"
#include "MapInterface.h"
#include "ColorShaderInterface.h"
#include "GraphicsObjectInterface.h"
#include "RenderPass.h"
#include <map>

PolygonLayer::PolygonLayer() {

}

void PolygonLayer::setPolygons(const std::vector<Polygon> & polygons) {
    clear();
    for (auto const &polygon: polygons) {
        add(polygon);
    }
    buildRenderPasses();
    mapInterface->invalidate();
}

std::vector<Polygon> PolygonLayer::getPolygons() {
    std::vector<Polygon> polygons;
    for (auto const &polygon : this->polygons) {
        polygons.push_back(polygon.first);
    }
    return polygons;
}

void PolygonLayer::remove(const Polygon & polygon) {
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        for(auto it = polygons.begin(); it != polygons.end(); it++) {
            if (it->first.identifier == polygon.identifier) {
                polygons.erase(it);
                break;
            }
        }
    }
    buildRenderPasses();
    mapInterface->invalidate();
}

void PolygonLayer::add(const Polygon & polygon) {
    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    auto shader = shaderFactory->createColorShader();
    shader->setColor(polygon.color.r, polygon.color.g, polygon.color.b, polygon.color.a);
    auto polygonGraphicsObject = objectFactory->createPolygon(shader->asShaderProgramInterface());

    auto polygonObject = std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(),
                                                                polygonGraphicsObject,
                                                                shader);

    polygonObject->setPositions(polygon.coordinates, polygon.holes, polygon.isConvex);
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        polygons[polygon] = polygonObject;
    }
    buildRenderPasses();
    mapInterface->invalidate();
}

void PolygonLayer::clear() {
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        polygons.clear();
    }
    buildRenderPasses();
    mapInterface->invalidate();
}

std::shared_ptr<::LayerInterface> PolygonLayer::asLayerInterface() {
    return shared_from_this();
}


void PolygonLayer::generateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
    std::map<int, std::vector<std::shared_ptr<GraphicsObjectInterface>>> renderPassObjectMap;
    for (auto const &polygonTuple : polygons) {
        for (auto config : polygonTuple.second->getRenderConfig()) {
            renderPassObjectMap[config->getRenderIndex()].push_back(config->getGraphicsObject());
        }
    }
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first),
                                                                              passEntry.second);
        newRenderPasses.push_back(renderPass);
    }
    renderPasses = newRenderPasses;
}

std::vector<std::shared_ptr<::RenderPassInterface>> PolygonLayer::buildRenderPasses() {
    return renderPasses;
}

void PolygonLayer::onAdded(const std::shared_ptr<MapInterface> & mapInterface) {
    this->mapInterface = mapInterface;
}
