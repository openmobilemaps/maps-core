//
// Created by Christoph Maurhofer on 08.02.2021.
//

#include "LambdaTask.h"
#include "Tiled2dMapSource.h"

Tiled2dMapSource::Tiled2dMapSource(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                   const std::shared_ptr<SchedulerInterface> &scheduler,
                                   const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
        : layerConfig(layerConfig),
          scheduler(scheduler),
          listener(listener),
          zoomInfo(layerConfig->getZoomLevelInfos()) {

}

void Tiled2dMapSource::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapSource_Update",
                       0,
                       TaskPriority::NORMAL,
                       ExecutionEnvironment::COMPUTATION),
            [=] { updateCurrentTileset(visibleBounds, zoom); }));
}

void Tiled2dMapSource::updateCurrentTileset(const RectCoord &visibleBounds, double zoom) {
    // TODO: update current tileset -> call onVisibleTilesChanged
}

