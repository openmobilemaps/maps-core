// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from camera.djinni

#import "MCMapCameraListenerInterface+Private.h"
#import "MCMapCameraListenerInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "DJIObjcWrapperCache+Private.h"
#import "MCCoord+Private.h"
#import "MCRectCoord+Private.h"
#import "MCVec3D+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCMapCameraListenerInterfaceCppProxy : NSObject<MCMapCameraListenerInterface>

- (id)initWithCpp:(const std::shared_ptr<::MapCameraListenerInterface>&)cppRef;

@end

@implementation MCMapCameraListenerInterfaceCppProxy {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::MapCameraListenerInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::MapCameraListenerInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

- (void)onVisibleBoundsChanged:(nonnull MCRectCoord *)visibleBounds
                          zoom:(double)zoom {
    try {
        _cppRefHandle.get()->onVisibleBoundsChanged(::djinni_generated::RectCoord::toCpp(visibleBounds),
                                                    ::djinni::F64::toCpp(zoom));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)onRotationChanged:(float)angle {
    try {
        _cppRefHandle.get()->onRotationChanged(::djinni::F32::toCpp(angle));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)onMapInteraction {
    try {
        _cppRefHandle.get()->onMapInteraction();
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)onCameraChange:(nonnull NSArray<NSNumber *> *)viewMatrix
      projectionMatrix:(nonnull NSArray<NSNumber *> *)projectionMatrix
                origin:(nonnull MCVec3D *)origin
           verticalFov:(float)verticalFov
         horizontalFov:(float)horizontalFov
                 width:(float)width
                height:(float)height
    focusPointAltitude:(float)focusPointAltitude
    focusPointPosition:(nonnull MCCoord *)focusPointPosition
                  zoom:(float)zoom {
    try {
        _cppRefHandle.get()->onCameraChange(::djinni::List<::djinni::F32>::toCpp(viewMatrix),
                                            ::djinni::List<::djinni::F32>::toCpp(projectionMatrix),
                                            ::djinni_generated::Vec3D::toCpp(origin),
                                            ::djinni::F32::toCpp(verticalFov),
                                            ::djinni::F32::toCpp(horizontalFov),
                                            ::djinni::F32::toCpp(width),
                                            ::djinni::F32::toCpp(height),
                                            ::djinni::F32::toCpp(focusPointAltitude),
                                            ::djinni_generated::Coord::toCpp(focusPointPosition),
                                            ::djinni::F32::toCpp(zoom));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

class MapCameraListenerInterface::ObjcProxy final
: public ::MapCameraListenerInterface
, private ::djinni::ObjcProxyBase<ObjcType>
{
    friend class ::djinni_generated::MapCameraListenerInterface;
public:
    using ObjcProxyBase::ObjcProxyBase;
    void onVisibleBoundsChanged(const ::RectCoord & c_visibleBounds, double c_zoom) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() onVisibleBoundsChanged:(::djinni_generated::RectCoord::fromCpp(c_visibleBounds))
                                                                        zoom:(::djinni::F64::fromCpp(c_zoom))];
        }
    }
    void onRotationChanged(float c_angle) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() onRotationChanged:(::djinni::F32::fromCpp(c_angle))];
        }
    }
    void onMapInteraction() override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() onMapInteraction];
        }
    }
    void onCameraChange(const std::vector<float> & c_viewMatrix, const std::vector<float> & c_projectionMatrix, const ::Vec3D & c_origin, float c_verticalFov, float c_horizontalFov, float c_width, float c_height, float c_focusPointAltitude, const ::Coord & c_focusPointPosition, float c_zoom) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() onCameraChange:(::djinni::List<::djinni::F32>::fromCpp(c_viewMatrix))
                                                    projectionMatrix:(::djinni::List<::djinni::F32>::fromCpp(c_projectionMatrix))
                                                              origin:(::djinni_generated::Vec3D::fromCpp(c_origin))
                                                         verticalFov:(::djinni::F32::fromCpp(c_verticalFov))
                                                       horizontalFov:(::djinni::F32::fromCpp(c_horizontalFov))
                                                               width:(::djinni::F32::fromCpp(c_width))
                                                              height:(::djinni::F32::fromCpp(c_height))
                                                  focusPointAltitude:(::djinni::F32::fromCpp(c_focusPointAltitude))
                                                  focusPointPosition:(::djinni_generated::Coord::fromCpp(c_focusPointPosition))
                                                                zoom:(::djinni::F32::fromCpp(c_zoom))];
        }
    }
};

} // namespace djinni_generated

namespace djinni_generated {

auto MapCameraListenerInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    if ([(id)objc isKindOfClass:[MCMapCameraListenerInterfaceCppProxy class]]) {
        return ((MCMapCameraListenerInterfaceCppProxy*)objc)->_cppRefHandle.get();
    }
    return ::djinni::get_objc_proxy<ObjcProxy>(objc);
}

auto MapCameraListenerInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    if (auto cppPtr = dynamic_cast<ObjcProxy*>(cpp.get())) {
        return cppPtr->djinni_private_get_proxied_objc_object();
    }
    return ::djinni::get_cpp_proxy<MCMapCameraListenerInterfaceCppProxy>(cpp);
}

} // namespace djinni_generated

@end
