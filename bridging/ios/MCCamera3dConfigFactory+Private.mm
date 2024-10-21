// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#import "MCCamera3dConfigFactory+Private.h"
#import "MCCamera3dConfigFactory.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "MCCamera3dConfig+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCCamera3dConfigFactory ()

- (id)initWithCpp:(const std::shared_ptr<::Camera3dConfigFactory>&)cppRef;

@end

@implementation MCCamera3dConfigFactory {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::Camera3dConfigFactory>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::Camera3dConfigFactory>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

+ (nonnull MCCamera3dConfig *)getBasicConfig {
    try {
        auto objcpp_result_ = ::Camera3dConfigFactory::getBasicConfig();
        return ::djinni_generated::Camera3dConfig::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

+ (nonnull MCCamera3dConfig *)getRestorConfig {
    try {
        auto objcpp_result_ = ::Camera3dConfigFactory::getRestorConfig();
        return ::djinni_generated::Camera3dConfig::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

auto Camera3dConfigFactory::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    return objc->_cppRefHandle.get();
}

auto Camera3dConfigFactory::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    return ::djinni::get_cpp_proxy<MCCamera3dConfigFactory>(cpp);
}

} // namespace djinni_generated

@end