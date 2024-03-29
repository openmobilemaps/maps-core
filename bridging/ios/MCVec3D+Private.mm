// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#import "MCVec3D+Private.h"
#import "DJIMarshal+Private.h"
#include <cassert>

namespace djinni_generated {

auto Vec3D::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::F64::toCpp(obj.x),
            ::djinni::F64::toCpp(obj.y),
            ::djinni::F64::toCpp(obj.z)};
}

auto Vec3D::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCVec3D alloc] initWithX:(::djinni::F64::fromCpp(cpp.x))
                                    y:(::djinni::F64::fromCpp(cpp.y))
                                    z:(::djinni::F64::fromCpp(cpp.z))];
}

} // namespace djinni_generated
