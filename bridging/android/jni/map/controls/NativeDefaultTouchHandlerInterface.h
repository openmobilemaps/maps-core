// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from touch_handler.djinni

#pragma once

#include "DefaultTouchHandlerInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeDefaultTouchHandlerInterface final : ::djinni::JniInterface<::DefaultTouchHandlerInterface, NativeDefaultTouchHandlerInterface> {
public:
    using CppType = std::shared_ptr<::DefaultTouchHandlerInterface>;
    using CppOptType = std::shared_ptr<::DefaultTouchHandlerInterface>;
    using JniType = jobject;

    using Boxed = NativeDefaultTouchHandlerInterface;

    ~NativeDefaultTouchHandlerInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeDefaultTouchHandlerInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeDefaultTouchHandlerInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeDefaultTouchHandlerInterface();
    friend ::djinni::JniClass<NativeDefaultTouchHandlerInterface>;
    friend ::djinni::JniInterface<::DefaultTouchHandlerInterface, NativeDefaultTouchHandlerInterface>;

};

} // namespace djinni_generated
