// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#pragma once

#include "ColorCircleShaderInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeColorCircleShaderInterface final : ::djinni::JniInterface<::ColorCircleShaderInterface, NativeColorCircleShaderInterface> {
public:
    using CppType = std::shared_ptr<::ColorCircleShaderInterface>;
    using CppOptType = std::shared_ptr<::ColorCircleShaderInterface>;
    using JniType = jobject;

    using Boxed = NativeColorCircleShaderInterface;

    ~NativeColorCircleShaderInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeColorCircleShaderInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeColorCircleShaderInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeColorCircleShaderInterface();
    friend ::djinni::JniClass<NativeColorCircleShaderInterface>;
    friend ::djinni::JniInterface<::ColorCircleShaderInterface, NativeColorCircleShaderInterface>;

};

} // namespace djinni_generated
