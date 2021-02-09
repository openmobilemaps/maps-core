#pragma once

#include "MapInterface.h"
#include "LayerInterface.h"
#include "GraphicsObjectFactoryInterface.h"
#include "ShaderFactoryInterface.h"
#include "Polygon2dLayerObject.h"
#include "Rectangle2dInterface.h"
#include "RenderPass.h"

class TestingLayer: public LayerInterface {
public:
    TestingLayer(const std::shared_ptr<MapInterface> &mapInterface);

    ~TestingLayer() {}

    std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses();

    std::string getIdentifier();

    void pause();

    void resume();

    void hide();

    void show();

private:
    std::shared_ptr<MapInterface> mapInterface;
    std::shared_ptr<Polygon2dLayerObject> polygonObject;
    std::shared_ptr<Rectangle2dInterface> rectangle;
    std::shared_ptr<Rectangle2dInterface> rectangle2;
    std::shared_ptr<Rectangle2dInterface> rectangle3;
    std::shared_ptr<Rectangle2dInterface> rectangle4;
    std::shared_ptr<RenderPass> renderPass;
};
