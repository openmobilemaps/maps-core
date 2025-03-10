// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#pragma once

#include "OpenGlPerformanceLoggerInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeOpenGlPerformanceLoggerInterface final : ::djinni::JniInterface<::OpenGlPerformanceLoggerInterface, NativeOpenGlPerformanceLoggerInterface> {
public:
    using CppType = std::shared_ptr<::OpenGlPerformanceLoggerInterface>;
    using CppOptType = std::shared_ptr<::OpenGlPerformanceLoggerInterface>;
    using JniType = jobject;

    using Boxed = NativeOpenGlPerformanceLoggerInterface;

    ~NativeOpenGlPerformanceLoggerInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeOpenGlPerformanceLoggerInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeOpenGlPerformanceLoggerInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeOpenGlPerformanceLoggerInterface();
    friend ::djinni::JniClass<NativeOpenGlPerformanceLoggerInterface>;
    friend ::djinni::JniInterface<::OpenGlPerformanceLoggerInterface, NativeOpenGlPerformanceLoggerInterface>;

};

} // namespace djinni_generated
