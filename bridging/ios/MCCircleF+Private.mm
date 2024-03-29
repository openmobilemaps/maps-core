// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#import "MCCircleF+Private.h"
#import "DJIMarshal+Private.h"
#include <cassert>

namespace djinni_generated {

auto CircleF::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::F32::toCpp(obj.x),
            ::djinni::F32::toCpp(obj.y),
            ::djinni::F32::toCpp(obj.radius)};
}

auto CircleF::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCCircleF alloc] initWithX:(::djinni::F32::fromCpp(cpp.x))
                                      y:(::djinni::F32::fromCpp(cpp.y))
                                 radius:(::djinni::F32::fromCpp(cpp.radius))];
}

} // namespace djinni_generated
