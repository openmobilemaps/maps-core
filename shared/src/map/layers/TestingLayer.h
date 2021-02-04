#pragma once

#include "MapInterface.h"
#include "LayerInterface.h"
#include "GraphicsObjectFactoryInterface.h"
#include "ShaderFactoryInterface.h"
#include "Polygon2dLayerObject.h"
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
    std::shared_ptr<Polygon2dLayerObject> object;
    std::shared_ptr<RenderPass> renderPass;
};
