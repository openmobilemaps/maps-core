// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#import "MCPolygonCoord+Private.h"
#import "DJIMarshal+Private.h"
#import "MCCoord+Private.h"
#include <cassert>

namespace djinni_generated {

auto PolygonCoord::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::List<::djinni_generated::Coord>::toCpp(obj.positions),
            ::djinni::List<::djinni::List<::djinni_generated::Coord>>::toCpp(obj.holes)};
}

auto PolygonCoord::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCPolygonCoord alloc] initWithPositions:(::djinni::List<::djinni_generated::Coord>::fromCpp(cpp.positions))
                                               holes:(::djinni::List<::djinni::List<::djinni_generated::Coord>>::fromCpp(cpp.holes))];
}

} // namespace djinni_generated
