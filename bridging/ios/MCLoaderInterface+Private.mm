// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from loader.djinni

#import "MCLoaderInterface+Private.h"
#import "MCLoaderInterface.h"
#import "DJICppWrapperCache+Private.h"
#import "DJIError.h"
#import "DJIMarshal+Private.h"
#import "DJIObjcWrapperCache+Private.h"
#import "Future_objc.hpp"
#import "MCDataLoaderResult+Private.h"
#import "MCTextureLoaderResult+Private.h"
#include <exception>
#include <stdexcept>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@interface MCLoaderInterfaceCppProxy : NSObject<MCLoaderInterface>

- (id)initWithCpp:(const std::shared_ptr<::LoaderInterface>&)cppRef;

@end

@implementation MCLoaderInterfaceCppProxy {
    ::djinni::CppProxyCache::Handle<std::shared_ptr<::LoaderInterface>> _cppRefHandle;
}

- (id)initWithCpp:(const std::shared_ptr<::LoaderInterface>&)cppRef
{
    if (self = [super init]) {
        _cppRefHandle.assign(cppRef);
    }
    return self;
}

- (nonnull MCTextureLoaderResult *)loadTexture:(nonnull NSString *)url
                                          etag:(nullable NSString *)etag {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->loadTexture(::djinni::String::toCpp(url),
                                                               ::djinni::Optional<std::optional, ::djinni::String>::toCpp(etag));
        return ::djinni_generated::TextureLoaderResult::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull MCDataLoaderResult *)loadData:(nonnull NSString *)url
                                    etag:(nullable NSString *)etag {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->loadData(::djinni::String::toCpp(url),
                                                            ::djinni::Optional<std::optional, ::djinni::String>::toCpp(etag));
        return ::djinni_generated::DataLoaderResult::fromCpp(objcpp_result_);
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull DJFuture<MCTextureLoaderResult *> *)loadTextureAsync:(nonnull NSString *)url
                                                           etag:(nullable NSString *)etag {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->loadTextureAsync(::djinni::String::toCpp(url),
                                                                    ::djinni::Optional<std::optional, ::djinni::String>::toCpp(etag));
        return ::djinni::FutureAdaptor<::djinni_generated::TextureLoaderResult>::fromCpp(std::move(objcpp_result_));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (nonnull DJFuture<MCDataLoaderResult *> *)loadDataAsync:(nonnull NSString *)url
                                                     etag:(nullable NSString *)etag {
    try {
        auto objcpp_result_ = _cppRefHandle.get()->loadDataAsync(::djinni::String::toCpp(url),
                                                                 ::djinni::Optional<std::optional, ::djinni::String>::toCpp(etag));
        return ::djinni::FutureAdaptor<::djinni_generated::DataLoaderResult>::fromCpp(std::move(objcpp_result_));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

- (void)cancel:(nonnull NSString *)url {
    try {
        _cppRefHandle.get()->cancel(::djinni::String::toCpp(url));
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

namespace djinni_generated {

class LoaderInterface::ObjcProxy final
: public ::LoaderInterface
, private ::djinni::ObjcProxyBase<ObjcType>
{
    friend class ::djinni_generated::LoaderInterface;
public:
    using ObjcProxyBase::ObjcProxyBase;
    ::TextureLoaderResult loadTexture(const std::string & c_url, const std::optional<std::string> & c_etag) override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() loadTexture:(::djinni::String::fromCpp(c_url))
                                                                                   etag:(::djinni::Optional<std::optional, ::djinni::String>::fromCpp(c_etag))];
            return ::djinni_generated::TextureLoaderResult::toCpp(objcpp_result_);
        }
    }
    ::DataLoaderResult loadData(const std::string & c_url, const std::optional<std::string> & c_etag) override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() loadData:(::djinni::String::fromCpp(c_url))
                                                                                etag:(::djinni::Optional<std::optional, ::djinni::String>::fromCpp(c_etag))];
            return ::djinni_generated::DataLoaderResult::toCpp(objcpp_result_);
        }
    }
    ::djinni::Future<::TextureLoaderResult> loadTextureAsync(const std::string & c_url, const std::optional<std::string> & c_etag) override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() loadTextureAsync:(::djinni::String::fromCpp(c_url))
                                                                                        etag:(::djinni::Optional<std::optional, ::djinni::String>::fromCpp(c_etag))];
            return ::djinni::FutureAdaptor<::djinni_generated::TextureLoaderResult>::toCpp(objcpp_result_);
        }
    }
    ::djinni::Future<::DataLoaderResult> loadDataAsync(const std::string & c_url, const std::optional<std::string> & c_etag) override
    {
        @autoreleasepool {
            auto objcpp_result_ = [djinni_private_get_proxied_objc_object() loadDataAsync:(::djinni::String::fromCpp(c_url))
                                                                                     etag:(::djinni::Optional<std::optional, ::djinni::String>::fromCpp(c_etag))];
            return ::djinni::FutureAdaptor<::djinni_generated::DataLoaderResult>::toCpp(objcpp_result_);
        }
    }
    void cancel(const std::string & c_url) override
    {
        @autoreleasepool {
            [djinni_private_get_proxied_objc_object() cancel:(::djinni::String::fromCpp(c_url))];
        }
    }
};

} // namespace djinni_generated

namespace djinni_generated {

auto LoaderInterface::toCpp(ObjcType objc) -> CppType
{
    if (!objc) {
        return nullptr;
    }
    if ([(id)objc isKindOfClass:[MCLoaderInterfaceCppProxy class]]) {
        return ((MCLoaderInterfaceCppProxy*)objc)->_cppRefHandle.get();
    }
    return ::djinni::get_objc_proxy<ObjcProxy>(objc);
}

auto LoaderInterface::fromCppOpt(const CppOptType& cpp) -> ObjcType
{
    if (!cpp) {
        return nil;
    }
    if (auto cppPtr = dynamic_cast<ObjcProxy*>(cpp.get())) {
        return cppPtr->djinni_private_get_proxied_objc_object();
    }
    return ::djinni::get_cpp_proxy<MCLoaderInterfaceCppProxy>(cpp);
}

} // namespace djinni_generated

@end
