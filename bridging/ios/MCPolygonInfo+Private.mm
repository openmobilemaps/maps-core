// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from polygon.djinni

#import "MCPolygonInfo+Private.h"
#import "DJIMarshal+Private.h"
#import "MCColor+Private.h"
#import "MCPolygonCoord+Private.h"
#include <cassert>

namespace djinni_generated {

auto PolygonInfo::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::String::toCpp(obj.identifier),
            ::djinni_generated::PolygonCoord::toCpp(obj.coordinates),
            ::djinni_generated::Color::toCpp(obj.color),
            ::djinni_generated::Color::toCpp(obj.highlightColor)};
}

auto PolygonInfo::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCPolygonInfo alloc] initWithIdentifier:(::djinni::String::fromCpp(cpp.identifier))
                                         coordinates:(::djinni_generated::PolygonCoord::fromCpp(cpp.coordinates))
                                               color:(::djinni_generated::Color::fromCpp(cpp.color))
                                      highlightColor:(::djinni_generated::Color::fromCpp(cpp.highlightColor))];
}

} // namespace djinni_generated
