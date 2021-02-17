#pragma once

#include "TextureLoaderInterface.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapSource.h"
#include "MapConfig.h"

class Tiled2dMapRasterSource : public Tiled2dMapSource<TextureHolderInterface> {
public:
    Tiled2dMapRasterSource(const MapConfig &mapConfig,
                           const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::shared_ptr<TextureLoaderInterface> &loader,
                           const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener);

    std::unordered_set<Tiled2dMapRasterTileInfo> getCurrentTiles();

    virtual void pause() override;

    virtual void resume() override;

protected:

    virtual void loadTile(Tiled2dMapTileInfo tile) override;

private:
    
    const std::shared_ptr<TextureLoaderInterface> loader;
};
