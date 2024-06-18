//
//  ReverseGeocoder.hpp
//  
//
//  Created by Stefan Mitterrutzner on 18.06.2024.
//

#include "ReverseGeocoder.h"

/*not-null*/ std::shared_ptr<ReverseGeocoderInterface> ReverseGeocoderInterface::create(const /*not-null*/ std::shared_ptr<::LoaderInterface> & loader, const std::string & tileUrlTemplate, int32_t zoomLevel) {
    return std::make_shared<ReverseGeocoder>(loader, tileUrlTemplate, zoomLevel);
}
