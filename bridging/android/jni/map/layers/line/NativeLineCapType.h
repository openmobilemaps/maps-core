// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from line.djinni

#pragma once

#include "LineCapType.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeLineCapType final : ::djinni::JniEnum {
public:
    using CppType = ::LineCapType;
    using JniType = jobject;

    using Boxed = NativeLineCapType;

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return static_cast<CppType>(::djinni::JniClass<NativeLineCapType>::get().ordinal(jniEnv, j)); }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, CppType c) { return ::djinni::JniClass<NativeLineCapType>::get().create(jniEnv, static_cast<jint>(c)); }

private:
    NativeLineCapType() : JniEnum("io/openmobilemaps/mapscore/shared/map/layers/line/LineCapType") {}
    friend ::djinni::JniClass<NativeLineCapType>;
};

} // namespace djinni_generated
