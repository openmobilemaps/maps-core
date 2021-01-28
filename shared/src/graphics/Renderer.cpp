#include "Renderer.h"

void Renderer::addToRenderQueue(const std::shared_ptr<RenderPassInterface> & renderPass) {
    renderQueue.push_back(renderPass);
}

/** Ensure calling on graphics thread */
void Renderer::drawFrame(const std::shared_ptr<RenderingContextInterface> & renderingContext,
                         const std::shared_ptr<CameraInterface> & camera) {

}
