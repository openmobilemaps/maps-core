// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from packer.djinni

#import "MCRectanglePackerPage+Private.h"
#import "DJIMarshal+Private.h"
#import "MCRectI+Private.h"
#include <cassert>

namespace djinni_generated {

auto RectanglePackerPage::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni::Map<::djinni::String, ::djinni_generated::RectI>::toCpp(obj.uvs)};
}

auto RectanglePackerPage::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCRectanglePackerPage alloc] initWithUvs:(::djinni::Map<::djinni::String, ::djinni_generated::RectI>::fromCpp(cpp.uvs))];
}

} // namespace djinni_generated
