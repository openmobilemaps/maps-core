// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from network_activity_manager.djinni

#import "MCNetworkActivityManagerInterface+Private.h"
#import "MCNetworkActivityManagerInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "MCNetworkActivityListenerInterface+Private.h"
#import "MCTiledLayerError+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCNetworkActivityManagerInterface ()

- (id)initWithCpp:(const std::shared_ptr<::NetworkActivityManagerInterface>&)cppRef;

@end

@implementation MCNetworkActivityManagerInterface {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::NetworkActivityManagerInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::NetworkActivityManagerInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

+ (nullable MCNetworkActivityManagerInterface *)create {
    try {
        auto objcpp_result_ = ::NetworkActivityManagerInterface::create();
        return ::djinni_generated::NetworkActivityManagerInterface::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)addTiledLayerError:(nonnull MCTiledLayerError *)error {
    try {
        _cppRefHandle.get()->addTiledLayerError(::djinni_generated::TiledLayerError::toCpp(error));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)removeError:(nonnull NSString *)url {
    try {
        _cppRefHandle.get()->removeError(::djinni::String::toCpp(url));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)removeAllErrorsForLayer:(nonnull NSString *)layerName {
    try {
        _cppRefHandle.get()->removeAllErrorsForLayer(::djinni::String::toCpp(layerName));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)clearAllErrors {
    try {
        _cppRefHandle.get()->clearAllErrors();
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)addNetworkActivityListener:(nullable id<MCNetworkActivityListenerInterface>)listener {
    try {
        _cppRefHandle.get()->addNetworkActivityListener(::djinni_generated::NetworkActivityListenerInterface::toCpp(listener));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)removeNetworkActivityListener:(nullable id<MCNetworkActivityListenerInterface>)listener {
    try {
        _cppRefHandle.get()->removeNetworkActivityListener(::djinni_generated::NetworkActivityListenerInterface::toCpp(listener));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)updateRemainingTasks:(nonnull NSString *)layerName
                   taskCount:(int32_t)taskCount {
    try {
        _cppRefHandle.get()->updateRemainingTasks(::djinni::String::toCpp(layerName),
                                                  ::djinni::I32::toCpp(taskCount));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

auto NetworkActivityManagerInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    return objc->_cppRefHandle.get();
}

auto NetworkActivityManagerInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    return ::djinni::get_cpp_proxy<MCNetworkActivityManagerInterface>(cpp);
}

}  // namespace djinni_generated

@end