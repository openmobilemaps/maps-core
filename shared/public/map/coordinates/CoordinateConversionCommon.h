#pragma once
#include <string>

class CoordinateConversionCommon {
public:
    
    static inline const std::string RENDER_SYSTEM_ID = "render_system";

    // WGS 84 / Pseudo-Mercator
    // https://epsg.io/3857
    static inline const std::string EPSG3857 = "EPSG:3857";

    // WGS 84
    // https://epsg.io/4326
    static inline const std::string EPSG4326 = "EPSG:4326";

    // LV03+
    // https://epsg.io/2056
    static inline const std::string EPSG2056 = "EPSG:2056";
};
