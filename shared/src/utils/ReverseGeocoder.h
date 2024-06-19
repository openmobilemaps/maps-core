//
//  ReverseGeocoder.hpp
//  
//
//  Created by Stefan Mitterrutzner on 18.06.2024.
//

#pragma once
#include "ReverseGeocoderInterface.h"

class ReverseGeocoder: public ReverseGeocoderInterface {
public:
    ReverseGeocoder(const /*not-null*/ std::shared_ptr<::LoaderInterface> & loader, const std::string & tileUrlTemplate, int32_t zoomLevel);

    virtual std::vector<::VectorLayerFeatureCoordInfo> reverseGeocode(const ::Coord & coord, int64_t thresholdMeters) override;

    virtual std::optional<::VectorLayerFeatureCoordInfo> reverseGeocodeClosest(const ::Coord & coord, int64_t thresholdMeters) override;

private:
    double distance(const Coord &c1, const Coord &c2);
    
    const /*not-null*/ std::shared_ptr<::LoaderInterface> loader;
    const std::string tileUrlTemplate;
    int32_t zoomLevel;
};

