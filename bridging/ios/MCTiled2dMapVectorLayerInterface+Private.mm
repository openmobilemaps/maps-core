// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from tiled_vector_layer.djinni

#import "MCTiled2dMapVectorLayerInterface+Private.h"
#import "MCTiled2dMapVectorLayerInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "MCFontLoaderInterface+Private.h"
#import "MCLayerInterface+Private.h"
#import "MCLoaderInterface+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCTiled2dMapVectorLayerInterface ()

- (id)initWithCpp:(const std::shared_ptr<::Tiled2dMapVectorLayerInterface>&)cppRef;

@end

@implementation MCTiled2dMapVectorLayerInterface {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::Tiled2dMapVectorLayerInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::Tiled2dMapVectorLayerInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

+ (nullable MCTiled2dMapVectorLayerInterface *)createFromStyleJson:(nonnull NSString *)path
                                                      vectorSource:(nonnull NSString *)vectorSource
                                                           loaders:(nonnull NSArray<id<MCLoaderInterface>> *)loaders
                                                        fontLoader:(nullable id<MCFontLoaderInterface>)fontLoader
                                                          dpFactor:(double)dpFactor {
    try {
        auto objcpp_result_ = ::Tiled2dMapVectorLayerInterface::createFromStyleJson(::djinni::String::toCpp(path),
                                                                                    ::djinni::String::toCpp(vectorSource),
                                                                                    ::djinni::List<::djinni_generated::LoaderInterface>::toCpp(loaders),
                                                                                    ::djinni_generated::FontLoaderInterface::toCpp(fontLoader),
                                                                                    ::djinni::F64::toCpp(dpFactor));
        return ::djinni_generated::Tiled2dMapVectorLayerInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nullable id<MCLayerInterface>)asLayerInterface {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->asLayerInterface();
        return ::djinni_generated::LayerInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

auto Tiled2dMapVectorLayerInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    return objc->_cppRefHandle.get();
}

auto Tiled2dMapVectorLayerInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    return ::djinni::get_cpp_proxy<MCTiled2dMapVectorLayerInterface>(cpp);
}

}  // namespace djinni_generated

@end