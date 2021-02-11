#include "Renderer.h"
#include "RenderPassInterface.h"
#include "CameraInterface.h"

void Renderer::addToRenderQueue(const std::shared_ptr<RenderPassInterface> & renderPass) {
    renderQueue.push(renderPass);
}

/** Ensure calling on graphics thread */
void Renderer::drawFrame(const std::shared_ptr<RenderingContextInterface> & renderingContext,
                         const std::shared_ptr<CameraInterface> & camera) {

    auto mvpMatrix = camera->getMvpMatrix();
    auto mvpMatrixPointer = (int64_t) mvpMatrix.data();

    renderingContext->setupDrawFrame();

    while (!renderQueue.empty())
    {
        auto pass = renderQueue.front();

        for (const auto &object: pass->getGraphicsObjects()) {
            object->render(renderingContext, pass->getRenderPassConfig(), mvpMatrixPointer);
        }

        renderQueue.pop();
    }
}
