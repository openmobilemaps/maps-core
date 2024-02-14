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

std::shared_ptr<Tiled2dMapLayerConfig> DefaultTiled2dMapLayerConfigs::webMercator(const std::string & layerName, const std::string & urlFormat) {
    return std::make_shared<WebMercatorTiled2dMapLayerConfig>(layerName, urlFormat);
}

#endif /* DefaultTiled2dMapLayerConfigs_h */
