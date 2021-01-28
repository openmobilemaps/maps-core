//
// Created by Christoph on 27.01.2021.
//

#include "SceneInterface.h"

std::shared_ptr <SceneInterface> SceneInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                      const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) {
    return NULL;
}

std::shared_ptr <SceneInterface> SceneInterface::createWithOpenGl() {
    return NULL;
}
