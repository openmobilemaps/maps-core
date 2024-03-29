// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from wmts_capabilities.djinni

#import "MCWmtsTiled2dMapLayerConfigFactory+Private.h"
#import "MCWmtsTiled2dMapLayerConfigFactory.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "MCTiled2dMapLayerConfig+Private.h"
#import "MCTiled2dMapZoomInfo+Private.h"
#import "MCTiled2dMapZoomLevelInfo+Private.h"
#import "MCWmtsLayerDescription+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCWmtsTiled2dMapLayerConfigFactory ()

- (id)initWithCpp:(const std::shared_ptr<::WmtsTiled2dMapLayerConfigFactory>&)cppRef;

@end

@implementation MCWmtsTiled2dMapLayerConfigFactory {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::WmtsTiled2dMapLayerConfigFactory>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::WmtsTiled2dMapLayerConfigFactory>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

+ (nullable id<MCTiled2dMapLayerConfig>)create:(nonnull MCWmtsLayerDescription *)wmtsLayerConfiguration
                                 zoomLevelInfo:(nonnull NSArray<MCTiled2dMapZoomLevelInfo *> *)zoomLevelInfo
                                      zoomInfo:(nonnull MCTiled2dMapZoomInfo *)zoomInfo
                    coordinateSystemIdentifier:(int32_t)coordinateSystemIdentifier
                           matrixSetIdentifier:(nonnull NSString *)matrixSetIdentifier {
    try {
        auto objcpp_result_ = ::WmtsTiled2dMapLayerConfigFactory::create(::djinni_generated::WmtsLayerDescription::toCpp(wmtsLayerConfiguration),
                                                                         ::djinni::List<::djinni_generated::Tiled2dMapZoomLevelInfo>::toCpp(zoomLevelInfo),
                                                                         ::djinni_generated::Tiled2dMapZoomInfo::toCpp(zoomInfo),
                                                                         ::djinni::I32::toCpp(coordinateSystemIdentifier),
                                                                         ::djinni::String::toCpp(matrixSetIdentifier));
        return ::djinni_generated::Tiled2dMapLayerConfig::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

auto WmtsTiled2dMapLayerConfigFactory::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    return objc->_cppRefHandle.get();
}

auto WmtsTiled2dMapLayerConfigFactory::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    return ::djinni::get_cpp_proxy<MCWmtsTiled2dMapLayerConfigFactory>(cpp);
}

} // namespace djinni_generated

@end
