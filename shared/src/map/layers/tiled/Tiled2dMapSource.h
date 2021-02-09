//
// Created by Christoph Maurhofer on 08.02.2021.
//

#pragma once

#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "LoaderInterface.h"
#include "SchedulerInterface.h"
#include "Coord.h"

class Tiled2dMapSource : public Tiled2dMapSourceInterface {
public:
    Tiled2dMapSource(const std::shared_ptr <Tiled2dMapLayerConfig> &layerConfig,
                     const std::shared_ptr <SchedulerInterface> &scheduler,
                     const std::shared_ptr <LoaderInterface> &loader);

    virtual void onVisibleRectChanged(const Coord &topLeft, const Coord &bottomRight, double zoom);

protected:
    virtual void loadTile(int left, int top, int zoom) = 0;

    std::shared_ptr <Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr <SchedulerInterface> scheduler;
    std::shared_ptr <LoaderInterface> loader;
};