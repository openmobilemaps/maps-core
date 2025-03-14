// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from icosahedron.djinni

#pragma once

#include "IcosahedronLayerInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeIcosahedronLayerInterface final : ::djinni::JniInterface<::IcosahedronLayerInterface, NativeIcosahedronLayerInterface> {
public:
    using CppType = std::shared_ptr<::IcosahedronLayerInterface>;
    using CppOptType = std::shared_ptr<::IcosahedronLayerInterface>;
    using JniType = jobject;

    using Boxed = NativeIcosahedronLayerInterface;

    ~NativeIcosahedronLayerInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeIcosahedronLayerInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeIcosahedronLayerInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeIcosahedronLayerInterface();
    friend ::djinni::JniClass<NativeIcosahedronLayerInterface>;
    friend ::djinni::JniInterface<::IcosahedronLayerInterface, NativeIcosahedronLayerInterface>;

};

} // namespace djinni_generated
