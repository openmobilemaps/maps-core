//
// Created by Christoph on 27.01.2021.
//

#include "Scene.h"

std::shared_ptr <Scene> Scene::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                      const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) {
    return NULL;
}

std::shared_ptr <Scene> Scene::createWithOpenGl() {
    return NULL;
}
