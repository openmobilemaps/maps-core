//
// Created by Christoph Maurhofer on 08.02.2021.
//

#include "Tiled2dMapSource.h"

Tiled2dMapSource::Tiled2dMapSource(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                            const std::shared_ptr<SchedulerInterface> &scheduler,
                                            const std::shared_ptr<LoaderInterface> &loader) : layerConfig(layerConfig),
                                                                                                       scheduler(scheduler),
                                                                                                       loader(loader) {

}

void Tiled2dMapSource::onVisibleRectChanged(const Coord &topLeft, const Coord &bottomRight, double zoom) {

}

