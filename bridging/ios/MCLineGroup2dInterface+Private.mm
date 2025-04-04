// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#import "MCLineGroup2dInterface+Private.h"
#import "MCLineGroup2dInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "DJIObjcWrapperCache+Private.h"
#import "MCGraphicsObjectInterface+Private.h"
#import "MCSharedBytes+Private.h"
#import "MCVec3D+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCLineGroup2dInterfaceCppProxy : NSObject<MCLineGroup2dInterface>

- (id)initWithCpp:(const std::shared_ptr<::LineGroup2dInterface>&)cppRef;

@end

@implementation MCLineGroup2dInterfaceCppProxy {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::LineGroup2dInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::LineGroup2dInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

- (void)setLines:(nonnull MCSharedBytes *)lines
         indices:(nonnull MCSharedBytes *)indices
          origin:(nonnull MCVec3D *)origin
            is3d:(BOOL)is3d {
    try {
        _cppRefHandle.get()->setLines(::djinni_generated::SharedBytes::toCpp(lines),
                                      ::djinni_generated::SharedBytes::toCpp(indices),
                                      ::djinni_generated::Vec3D::toCpp(origin),
                                      ::djinni::Bool::toCpp(is3d));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nullable id<MCGraphicsObjectInterface>)asGraphicsObject {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->asGraphicsObject();
        return ::djinni_generated::GraphicsObjectInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

class LineGroup2dInterface::ObjcProxy final
: public ::LineGroup2dInterface
, private ::djinni::ObjcProxyBase<ObjcType>
{
    friend class ::djinni_generated::LineGroup2dInterface;
public:
    using ObjcProxyBase::ObjcProxyBase;
    void setLines(const ::SharedBytes & c_lines, const ::SharedBytes & c_indices, const ::Vec3D & c_origin, bool c_is3d) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() setLines:(::djinni_generated::SharedBytes::fromCpp(c_lines))
                                                       indices:(::djinni_generated::SharedBytes::fromCpp(c_indices))
                                                        origin:(::djinni_generated::Vec3D::fromCpp(c_origin))
                                                          is3d:(::djinni::Bool::fromCpp(c_is3d))];
        }
    }
    /*not-null*/ std::shared_ptr<::GraphicsObjectInterface> asGraphicsObject() override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() asGraphicsObject];
            return ::djinni_generated::GraphicsObjectInterface::toCpp(objcpp_result_);
        }
    }
};

} // namespace djinni_generated

namespace djinni_generated {

auto LineGroup2dInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    if ([(id)objc isKindOfClass:[MCLineGroup2dInterfaceCppProxy class]]) {
        return ((MCLineGroup2dInterfaceCppProxy*)objc)->_cppRefHandle.get();
    }
    return ::djinni::get_objc_proxy<ObjcProxy>(objc);
}

auto LineGroup2dInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    if (auto cppPtr = dynamic_cast<ObjcProxy*>(cpp.get())) {
        return cppPtr->djinni_private_get_proxied_objc_object();
    }
    return ::djinni::get_cpp_proxy<MCLineGroup2dInterfaceCppProxy>(cpp);
}

} // namespace djinni_generated

@end
