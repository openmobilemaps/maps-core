//
//  GeoJsonFeatureParserInterface.cpp
//  
//
//  Created by Bastian Morath on 22.08.23.
//

#include "GeoJsonFeatureParser.h"

std::shared_ptr<GeoJsonFeatureParserInterface> GeoJsonFeatureParserInterface::create() { return std::make_shared<GeoJsonFeatureParser>(); }
