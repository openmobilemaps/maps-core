#include "PolygonLayer.h"
#include "ColorShaderInterface.h"
#include "GraphicsObjectInterface.h"
#include "MapInterface.h"
#include "RenderPass.h"
#include <map>

PolygonLayer::PolygonLayer()
    : isHidden(false) {}

void PolygonLayer::setPolygons(const std::vector<Polygon> &polygons) {
    clear();
    for (auto const &polygon : polygons) {
        add(polygon);
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

std::vector<Polygon> PolygonLayer::getPolygons() {
    std::vector<Polygon> polygons;
    if (!mapInterface) {
        for (auto const &polygon : addingQueue) {
            polygons.push_back(polygon);
        }
        return polygons;
    }
    for (auto const &polygon : this->polygons) {
        polygons.push_back(polygon.first);
    }
    return polygons;
}

void PolygonLayer::remove(const Polygon &polygon) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.erase(polygon);
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        for (auto it = polygons.begin(); it != polygons.end(); it++) {
            if (it->first.identifier == polygon.identifier) {
                polygons.erase(it);
                break;
            }
        }
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void PolygonLayer::add(const Polygon &polygon) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.insert(polygon);
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    auto shader = shaderFactory->createColorShader();
    shader->setColor(polygon.color.r, polygon.color.g, polygon.color.b, polygon.color.a);
    auto polygonGraphicsObject = objectFactory->createPolygon(shader->asShaderProgramInterface());

    polygonGraphicsObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

    auto polygonObject =
        std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(), polygonGraphicsObject, shader);

    polygonObject->setPositions(polygon.coordinates, polygon.holes, polygon.isConvex);
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        polygons[polygon] = polygonObject;
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void PolygonLayer::clear() {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        polygons.clear();
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void PolygonLayer::pause() {
    std::lock_guard<std::recursive_mutex> overlayLock(polygonsMutex);
    for (const auto &polygon : polygons) {
        polygon.second->getPolygonObject()->clear();
    }
}

void PolygonLayer::resume() {
    std::lock_guard<std::recursive_mutex> overlayLock(polygonsMutex);
    for (const auto &polygon : polygons) {
        polygon.second->getPolygonObject()->setup(mapInterface->getRenderingContext());
    }
}

std::shared_ptr<::LayerInterface> PolygonLayer::asLayerInterface() { return shared_from_this(); }

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
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second);
        newRenderPasses.push_back(renderPass);
    }
    renderPasses = newRenderPasses;
}

std::vector<std::shared_ptr<::RenderPassInterface>> PolygonLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        return renderPasses;
    }
}

void PolygonLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto const &polygon : addingQueue) {
            add(polygon);
        }
        addingQueue.clear();
    }
}

void PolygonLayer::hide() {
    isHidden = true;
    mapInterface->invalidate();
}

void PolygonLayer::show() {
    isHidden = false;
    mapInterface->invalidate();
}
