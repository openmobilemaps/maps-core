// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from loader.djinni

#pragma once

#include "LoaderStatus.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeLoaderStatus final : ::djinni::JniEnum {
public:
    using CppType = ::LoaderStatus;
    using JniType = jobject;

    using Boxed = NativeLoaderStatus;

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return static_cast<CppType>(::djinni::JniClass<NativeLoaderStatus>::get().ordinal(jniEnv, j)); }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, CppType c) { return ::djinni::JniClass<NativeLoaderStatus>::get().create(jniEnv, static_cast<jint>(c)); }

private:
    NativeLoaderStatus() : JniEnum("io/openmobilemaps/mapscore/shared/map/loader/LoaderStatus") {}
    friend ::djinni::JniClass<NativeLoaderStatus>;
};

} // namespace djinni_generated
