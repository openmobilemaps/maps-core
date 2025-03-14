// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#import "MCRendererInterface+Private.h"
#import "MCRendererInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "DJIObjcWrapperCache+Private.h"
#import "MCCameraInterface+Private.h"
#import "MCComputePassInterface+Private.h"
#import "MCRenderPassInterface+Private.h"
#import "MCRenderTargetInterface+Private.h"
#import "MCRenderingContextInterface+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCRendererInterfaceCppProxy : NSObject<MCRendererInterface>

- (id)initWithCpp:(const std::shared_ptr<::RendererInterface>&)cppRef;

@end

@implementation MCRendererInterfaceCppProxy {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::RendererInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::RendererInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

- (void)addToRenderQueue:(nullable id<MCRenderPassInterface>)renderPass {
    try {
        _cppRefHandle.get()->addToRenderQueue(::djinni_generated::RenderPassInterface::toCpp(renderPass));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)addToComputeQueue:(nullable id<MCComputePassInterface>)computePass {
    try {
        _cppRefHandle.get()->addToComputeQueue(::djinni_generated::ComputePassInterface::toCpp(computePass));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)drawFrame:(nullable id<MCRenderingContextInterface>)renderingContext
           camera:(nullable id<MCCameraInterface>)camera
           target:(nullable id<MCRenderTargetInterface>)target {
    try {
        _cppRefHandle.get()->drawFrame(::djinni_generated::RenderingContextInterface::toCpp(renderingContext),
                                       ::djinni_generated::CameraInterface::toCpp(camera),
                                       ::djinni::Optional<std::optional, ::djinni_generated::RenderTargetInterface>::toCpp(target));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)compute:(nullable id<MCRenderingContextInterface>)renderingContext
         camera:(nullable id<MCCameraInterface>)camera {
    try {
        _cppRefHandle.get()->compute(::djinni_generated::RenderingContextInterface::toCpp(renderingContext),
                                     ::djinni_generated::CameraInterface::toCpp(camera));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

class RendererInterface::ObjcProxy final
: public ::RendererInterface
, private ::djinni::ObjcProxyBase<ObjcType>
{
    friend class ::djinni_generated::RendererInterface;
public:
    using ObjcProxyBase::ObjcProxyBase;
    void addToRenderQueue(const /*not-null*/ std::shared_ptr<::RenderPassInterface> & c_renderPass) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() addToRenderQueue:(::djinni_generated::RenderPassInterface::fromCpp(c_renderPass))];
        }
    }
    void addToComputeQueue(const /*not-null*/ std::shared_ptr<::ComputePassInterface> & c_computePass) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() addToComputeQueue:(::djinni_generated::ComputePassInterface::fromCpp(c_computePass))];
        }
    }
    void drawFrame(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_renderingContext, const /*not-null*/ std::shared_ptr<::CameraInterface> & c_camera, const /*nullable*/ std::shared_ptr<::RenderTargetInterface> & c_target) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() drawFrame:(::djinni_generated::RenderingContextInterface::fromCpp(c_renderingContext))
                                                         camera:(::djinni_generated::CameraInterface::fromCpp(c_camera))
                                                         target:(::djinni::Optional<std::optional, ::djinni_generated::RenderTargetInterface>::fromCpp(c_target))];
        }
    }
    void compute(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_renderingContext, const /*not-null*/ std::shared_ptr<::CameraInterface> & c_camera) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() compute:(::djinni_generated::RenderingContextInterface::fromCpp(c_renderingContext))
                                                       camera:(::djinni_generated::CameraInterface::fromCpp(c_camera))];
        }
    }
};

} // namespace djinni_generated

namespace djinni_generated {

auto RendererInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    if ([(id)objc isKindOfClass:[MCRendererInterfaceCppProxy class]]) {
        return ((MCRendererInterfaceCppProxy*)objc)->_cppRefHandle.get();
    }
    return ::djinni::get_objc_proxy<ObjcProxy>(objc);
}

auto RendererInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    if (auto cppPtr = dynamic_cast<ObjcProxy*>(cpp.get())) {
        return cppPtr->djinni_private_get_proxied_objc_object();
    }
    return ::djinni::get_cpp_proxy<MCRendererInterfaceCppProxy>(cpp);
}

} // namespace djinni_generated

@end
