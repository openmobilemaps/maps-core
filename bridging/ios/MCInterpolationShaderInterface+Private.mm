// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from shader.djinni

#import "MCInterpolationShaderInterface+Private.h"
#import "MCInterpolationShaderInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "DJIObjcWrapperCache+Private.h"
#import "MCAlphaShaderInterface+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCInterpolationShaderInterfaceCppProxy : NSObject<MCInterpolationShaderInterface>

- (id)initWithCpp:(const std::shared_ptr<::InterpolationShaderInterface>&)cppRef;

@end

@implementation MCInterpolationShaderInterfaceCppProxy {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::InterpolationShaderInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::InterpolationShaderInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

- (void)updateFraction:(float)value {
    try {
        _cppRefHandle.get()->updateFraction(::djinni::F32::toCpp(value));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nullable id<MCAlphaShaderInterface>)asAlphaShaderInterface {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->asAlphaShaderInterface();
        return ::djinni_generated::AlphaShaderInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

class InterpolationShaderInterface::ObjcProxy final
: public ::InterpolationShaderInterface
, private ::djinni::ObjcProxyBase<ObjcType>
{
    friend class ::djinni_generated::InterpolationShaderInterface;
public:
    using ObjcProxyBase::ObjcProxyBase;
    void updateFraction(float c_value) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() updateFraction:(::djinni::F32::fromCpp(c_value))];
        }
    }
    std::shared_ptr<::AlphaShaderInterface> asAlphaShaderInterface() override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() asAlphaShaderInterface];
            return ::djinni_generated::AlphaShaderInterface::toCpp(objcpp_result_);
        }
    }
};

}  // namespace djinni_generated

namespace djinni_generated {

auto InterpolationShaderInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    if ([(id)objc isKindOfClass:[MCInterpolationShaderInterfaceCppProxy class]]) {
        return ((MCInterpolationShaderInterfaceCppProxy*)objc)->_cppRefHandle.get();
    }
    return ::djinni::get_objc_proxy<ObjcProxy>(objc);
}

auto InterpolationShaderInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    if (auto cppPtr = dynamic_cast<ObjcProxy*>(cpp.get())) {
        return cppPtr->djinni_private_get_proxied_objc_object();
    }
    return ::djinni::get_cpp_proxy<MCInterpolationShaderInterfaceCppProxy>(cpp);
}

}  // namespace djinni_generated

@end