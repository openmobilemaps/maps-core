// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from loader.djinni

#include "NativeTextureLoaderResult.h"  // my header
#include "Marshal.hpp"
#include "NativeLoaderStatus.h"
#include "NativeTextureHolderInterface.h"

namespace djinni_generated {

NativeTextureLoaderResult::NativeTextureLoaderResult() = default;

NativeTextureLoaderResult::~NativeTextureLoaderResult() = default;

auto NativeTextureLoaderResult::fromCpp(JNIEnv* jniEnv, const CppType& c) -> ::djinni::LocalRef<JniType> {
    const auto& data = ::djinni::JniClass<NativeTextureLoaderResult>::get();
    auto r = ::djinni::LocalRef<JniType>{jniEnv->NewObject(data.clazz.get(), data.jconstructor,
                                                           ::djinni::get(::djinni::Optional<std::optional, ::djinni_generated::NativeTextureHolderInterface>::fromCpp(jniEnv, c.data)),
                                                           ::djinni::get(::djinni::Optional<std::optional, ::djinni::String>::fromCpp(jniEnv, c.etag)),
                                                           ::djinni::get(::djinni_generated::NativeLoaderStatus::fromCpp(jniEnv, c.status)),
                                                           ::djinni::get(::djinni::Optional<std::optional, ::djinni::String>::fromCpp(jniEnv, c.errorCode)))};
    ::djinni::jniExceptionCheck(jniEnv);
    return r;
}

auto NativeTextureLoaderResult::toCpp(JNIEnv* jniEnv, JniType j) -> CppType {
    ::djinni::JniLocalScope jscope(jniEnv, 5);
    assert(j != nullptr);
    const auto& data = ::djinni::JniClass<NativeTextureLoaderResult>::get();
    return {::djinni::Optional<std::optional, ::djinni_generated::NativeTextureHolderInterface>::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_data)),
            ::djinni::Optional<std::optional, ::djinni::String>::toCpp(jniEnv, (jstring)jniEnv->GetObjectField(j, data.field_etag)),
            ::djinni_generated::NativeLoaderStatus::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_status)),
            ::djinni::Optional<std::optional, ::djinni::String>::toCpp(jniEnv, (jstring)jniEnv->GetObjectField(j, data.field_errorCode))};
}

} // namespace djinni_generated
