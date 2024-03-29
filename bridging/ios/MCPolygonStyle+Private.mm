// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from polygon.djinni

#import "MCPolygonStyle+Private.h"
#import "DJIMarshal+Private.h"
#import "MCColor+Private.h"
#include <cassert>

namespace djinni_generated {

auto PolygonStyle::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni_generated::Color::toCpp(obj.color),
            ::djinni::F32::toCpp(obj.opacity)};
}

auto PolygonStyle::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCPolygonStyle alloc] initWithColor:(::djinni_generated::Color::fromCpp(cpp.color))
                                         opacity:(::djinni::F32::fromCpp(cpp.opacity))];
}

} // namespace djinni_generated
