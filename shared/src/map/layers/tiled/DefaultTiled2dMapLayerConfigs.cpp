//
//  DefaultTiled2dMapLayerConfigs.h
//
//
//  Created by Nicolas Märki on 13.02.2024.
//

#ifndef DefaultTiled2dMapLayerConfigs_h
#define DefaultTiled2dMapLayerConfigs_h

#include "DefaultTiled2dMapLayerConfigs.h"
#include "WebMercatorTiled2dMapLayerConfig.h"
#include "Epsg4326Tiled2dMapLayerConfig.h"

std::shared_ptr<Tiled2dMapLayerConfig> DefaultTiled2dMapLayerConfigs::webMercator(const std::string & layerName, const std::string & urlFormat) {
    return std::make_shared<WebMercatorTiled2dMapLayerConfig>(layerName, urlFormat);
}

std::shared_ptr<Tiled2dMapLayerConfig>
DefaultTiled2dMapLayerConfigs::webMercatorCustom(const std::string &layerName, const std::string &urlFormat,
                                                 const Tiled2dMapZoomInfo &zoomInfo, int32_t minZoomLevel, int32_t maxZoomLevel) {
    return std::make_shared<WebMercatorTiled2dMapLayerConfig>(layerName, urlFormat, zoomInfo, minZoomLevel, maxZoomLevel);
}

std::shared_ptr<Tiled2dMapLayerConfig>
DefaultTiled2dMapLayerConfigs::epsg4326(const std::string &layerName, const std::string &urlFormat) {
    return std::make_shared<Epsg4326Tiled2dMapLayerConfig>(layerName, urlFormat);
}

std::shared_ptr<Tiled2dMapLayerConfig>
DefaultTiled2dMapLayerConfigs::epsg4326Custom(const std::string &layerName, const std::string &urlFormat,
                                              const Tiled2dMapZoomInfo &zoomInfo, int32_t minZoomLevel, int32_t maxZoomLevel) {
    return std::make_shared<Epsg4326Tiled2dMapLayerConfig>(layerName, urlFormat, zoomInfo, minZoomLevel, maxZoomLevel);
}

#endif /* DefaultTiled2dMapLayerConfigs_h */
