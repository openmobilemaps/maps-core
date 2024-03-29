// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from line.djinni

#pragma once

#include "LineLayerInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeLineLayerInterface final : ::djinni::JniInterface<::LineLayerInterface, NativeLineLayerInterface> {
public:
    using CppType = std::shared_ptr<::LineLayerInterface>;
    using CppOptType = std::shared_ptr<::LineLayerInterface>;
    using JniType = jobject;

    using Boxed = NativeLineLayerInterface;

    ~NativeLineLayerInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeLineLayerInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeLineLayerInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeLineLayerInterface();
    friend ::djinni::JniClass<NativeLineLayerInterface>;
    friend ::djinni::JniInterface<::LineLayerInterface, NativeLineLayerInterface>;

};

} // namespace djinni_generated
