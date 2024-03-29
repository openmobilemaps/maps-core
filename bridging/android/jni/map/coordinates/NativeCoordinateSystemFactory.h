// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#pragma once

#include "CoordinateSystemFactory.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeCoordinateSystemFactory final : ::djinni::JniInterface<::CoordinateSystemFactory, NativeCoordinateSystemFactory> {
public:
    using CppType = std::shared_ptr<::CoordinateSystemFactory>;
    using CppOptType = std::shared_ptr<::CoordinateSystemFactory>;
    using JniType = jobject;

    using Boxed = NativeCoordinateSystemFactory;

    ~NativeCoordinateSystemFactory();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeCoordinateSystemFactory>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeCoordinateSystemFactory>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeCoordinateSystemFactory();
    friend ::djinni::JniClass<NativeCoordinateSystemFactory>;
    friend ::djinni::JniInterface<::CoordinateSystemFactory, NativeCoordinateSystemFactory>;

};

} // namespace djinni_generated
