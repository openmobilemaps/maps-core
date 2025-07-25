// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from line.djinni

#import "MCLineStyle+Private.h"
#import "DJIMarshal+Private.h"
#import "MCColorStateList+Private.h"
#import "MCLineCapType+Private.h"
#import "MCLineJoinType+Private.h"
#import "MCSizeType+Private.h"
#include <cassert>

namespace djinni_generated {

auto LineStyle::toCpp(ObjcType obj) -> CppType
{
    assert(obj);
    return {::djinni_generated::ColorStateList::toCpp(obj.color),
            ::djinni_generated::ColorStateList::toCpp(obj.gapColor),
            ::djinni::F32::toCpp(obj.opacity),
            ::djinni::F32::toCpp(obj.blur),
            ::djinni::Enum<::SizeType, MCSizeType>::toCpp(obj.widthType),
            ::djinni::F32::toCpp(obj.width),
            ::djinni::List<::djinni::F32>::toCpp(obj.dashArray),
            ::djinni::F32::toCpp(obj.dashFade),
            ::djinni::F32::toCpp(obj.dashAnimationSpeed),
            ::djinni::Enum<::LineCapType, MCLineCapType>::toCpp(obj.lineCap),
            ::djinni::Enum<::LineJoinType, MCLineJoinType>::toCpp(obj.lineJoin),
            ::djinni::F32::toCpp(obj.offset),
            ::djinni::Bool::toCpp(obj.dotted),
            ::djinni::F32::toCpp(obj.dottedSkew)};
}

auto LineStyle::fromCpp(const CppType& cpp) -> ObjcType
{
    return [[MCLineStyle alloc] initWithColor:(::djinni_generated::ColorStateList::fromCpp(cpp.color))
                                     gapColor:(::djinni_generated::ColorStateList::fromCpp(cpp.gapColor))
                                      opacity:(::djinni::F32::fromCpp(cpp.opacity))
                                         blur:(::djinni::F32::fromCpp(cpp.blur))
                                    widthType:(::djinni::Enum<::SizeType, MCSizeType>::fromCpp(cpp.widthType))
                                        width:(::djinni::F32::fromCpp(cpp.width))
                                    dashArray:(::djinni::List<::djinni::F32>::fromCpp(cpp.dashArray))
                                     dashFade:(::djinni::F32::fromCpp(cpp.dashFade))
                           dashAnimationSpeed:(::djinni::F32::fromCpp(cpp.dashAnimationSpeed))
                                      lineCap:(::djinni::Enum<::LineCapType, MCLineCapType>::fromCpp(cpp.lineCap))
                                     lineJoin:(::djinni::Enum<::LineJoinType, MCLineJoinType>::fromCpp(cpp.lineJoin))
                                       offset:(::djinni::F32::fromCpp(cpp.offset))
                                       dotted:(::djinni::Bool::fromCpp(cpp.dotted))
                                   dottedSkew:(::djinni::F32::fromCpp(cpp.dottedSkew))];
}

} // namespace djinni_generated
