// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from icosahedron.djinni

#import "MCIcosahedronLayerInterface+Private.h"
#import "MCIcosahedronLayerInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "MCIcosahedronLayerCallbackInterface+Private.h"
#import "MCLayerInterface+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCIcosahedronLayerInterface ()

- (id)initWithCpp:(const std::shared_ptr<::IcosahedronLayerInterface>&)cppRef;

@end

@implementation MCIcosahedronLayerInterface {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::IcosahedronLayerInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::IcosahedronLayerInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

+ (nullable MCIcosahedronLayerInterface *)create:(nullable id<MCIcosahedronLayerCallbackInterface>)callbackHandler {
    try {
        auto objcpp_result_ = ::IcosahedronLayerInterface::create(::djinni_generated::IcosahedronLayerCallbackInterface::toCpp(callbackHandler));
        return ::djinni_generated::IcosahedronLayerInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nullable id<MCLayerInterface>)asLayerInterface {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->asLayerInterface();
        return ::djinni_generated::LayerInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

auto IcosahedronLayerInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    return objc->_cppRefHandle.get();
}

auto IcosahedronLayerInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    return ::djinni::get_cpp_proxy<MCIcosahedronLayerInterface>(cpp);
}

} // namespace djinni_generated

@end