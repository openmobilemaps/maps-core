// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#pragma once

#include "ColorShaderInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeColorShaderInterface final : ::djinni::JniInterface<::ColorShaderInterface, NativeColorShaderInterface> {
public:
    using CppType = std::shared_ptr<::ColorShaderInterface>;
    using CppOptType = std::shared_ptr<::ColorShaderInterface>;
    using JniType = jobject;

    using Boxed = NativeColorShaderInterface;

    ~NativeColorShaderInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeColorShaderInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeColorShaderInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeColorShaderInterface();
    friend ::djinni::JniClass<NativeColorShaderInterface>;
    friend ::djinni::JniInterface<::ColorShaderInterface, NativeColorShaderInterface>;

};

} // namespace djinni_generated
