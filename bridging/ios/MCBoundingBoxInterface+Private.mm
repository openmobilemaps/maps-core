// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#import "MCBoundingBoxInterface+Private.h"
#import "MCBoundingBoxInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "MCCoord+Private.h"
#import "MCRectCoord+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCBoundingBoxInterface ()

- (id)initWithCpp:(const std::shared_ptr<::BoundingBoxInterface>&)cppRef;

@end

@implementation MCBoundingBoxInterface {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::BoundingBoxInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::BoundingBoxInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

+ (nullable MCBoundingBoxInterface *)create:(nonnull NSString *)systemIdentifier {
    try {
        auto objcpp_result_ = ::BoundingBoxInterface::create(::djinni::String::toCpp(systemIdentifier));
        return ::djinni_generated::BoundingBoxInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)addPoint:(nonnull MCCoord *)p {
    try {
        _cppRefHandle.get()->addPoint(::djinni_generated::Coord::toCpp(p));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull MCRectCoord *)asRectCoord {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->asRectCoord();
        return ::djinni_generated::RectCoord::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull MCCoord *)getCenter {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->getCenter();
        return ::djinni_generated::Coord::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull MCCoord *)getMin {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->getMin();
        return ::djinni_generated::Coord::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull MCCoord *)getMax {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->getMax();
        return ::djinni_generated::Coord::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull NSString *)getSystemIdentifier {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->getSystemIdentifier();
        return ::djinni::String::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

auto BoundingBoxInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    return objc->_cppRefHandle.get();
}

auto BoundingBoxInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    return ::djinni::get_cpp_proxy<MCBoundingBoxInterface>(cpp);
}

} // namespace djinni_generated

@end