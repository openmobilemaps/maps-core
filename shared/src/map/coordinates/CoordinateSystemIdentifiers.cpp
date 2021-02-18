#include "CoordinateSystemIdentifiers.h"

std::string CoordinateSystemIdentifiers::RENDERSYSTEM() { return "render_system"; };

// WGS 84 / Pseudo-Mercator
// https://epsg.io/3857
std::string CoordinateSystemIdentifiers::EPSG3857() { return "EPSG:3857"; };

// WGS 84
// https://epsg.io/4326
std::string CoordinateSystemIdentifiers::EPSG4326() { return "EPSG:4326"; };

// LV03+
// https://epsg.io/2056
std::string CoordinateSystemIdentifiers::EPSG2056() { return "EPSG:2056"; };
