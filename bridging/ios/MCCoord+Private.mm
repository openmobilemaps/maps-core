// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#import "MCCoord+Private.h"
#import "DJIMarshal+Private.h"
#include <cassert>

namespace djinni_generated {

auto Coord::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::I32::toCpp(obj.systemIdentifier),
            ::djinni::F64::toCpp(obj.x),
            ::djinni::F64::toCpp(obj.y),
            ::djinni::F64::toCpp(obj.z)};
}

auto Coord::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCCoord alloc] initWithSystemIdentifier:(::djinni::I32::fromCpp(cpp.systemIdentifier))
                                                   x:(::djinni::F64::fromCpp(cpp.x))
                                                   y:(::djinni::F64::fromCpp(cpp.y))
                                                   z:(::djinni::F64::fromCpp(cpp.z))];
}

} // namespace djinni_generated
