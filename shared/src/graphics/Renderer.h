#pragma once

#include "RendererInterface.h"
#include <vector>

class Renderer: public RendererInterface {
public:
    void addToRenderQueue(const std::shared_ptr<RenderPassInterface> & renderPass);

    /** Ensure calling on graphics thread */
    void drawFrame(const std::shared_ptr<RenderingContextInterface> & renderingContext,
                    const std::shared_ptr<CameraInterface> & camera);

private:
    std::vector<const std::shared_ptr<RenderPassInterface>> renderQueue;
};
