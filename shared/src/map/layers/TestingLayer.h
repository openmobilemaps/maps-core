#pragma once
#include "LayerInterface.h"
#include "GraphicsObjectFactoryInterface.h"
#include "ShaderFactoryInterface.h"
#include "Polygon2dLayerObject.h"
#include "RenderPass.h"

class TestingLayer: public LayerInterface {
public:
    TestingLayer(std::shared_ptr<GraphicsObjectFactoryInterface> graphicsObjectFactory, std::shared_ptr<ShaderFactoryInterface> shaderFactory);

    ~TestingLayer() {}

    std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses();

    std::string getIdentifier();

    void pause();

    void resume();

    void hide();

    void show();

private:
    std::shared_ptr<Polygon2dLayerObject> object;
    std::shared_ptr<RenderPass> renderPass;
};
