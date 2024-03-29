// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from touch_handler.djinni

#pragma once

#include "TouchAction.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeTouchAction final : ::djinni::JniEnum {
public:
    using CppType = ::TouchAction;
    using JniType = jobject;

    using Boxed = NativeTouchAction;

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return static_cast<CppType>(::djinni::JniClass<NativeTouchAction>::get().ordinal(jniEnv, j)); }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, CppType c) { return ::djinni::JniClass<NativeTouchAction>::get().create(jniEnv, static_cast<jint>(c)); }

private:
    NativeTouchAction() : JniEnum("io/openmobilemaps/mapscore/shared/map/controls/TouchAction") {}
    friend ::djinni::JniClass<NativeTouchAction>;
};

} // namespace djinni_generated
