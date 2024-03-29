// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from exception_logger.djinni

#pragma once

#include "ExceptionLoggerInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeExceptionLoggerInterface final : ::djinni::JniInterface<::ExceptionLoggerInterface, NativeExceptionLoggerInterface> {
public:
    using CppType = std::shared_ptr<::ExceptionLoggerInterface>;
    using CppOptType = std::shared_ptr<::ExceptionLoggerInterface>;
    using JniType = jobject;

    using Boxed = NativeExceptionLoggerInterface;

    ~NativeExceptionLoggerInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeExceptionLoggerInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeExceptionLoggerInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeExceptionLoggerInterface();
    friend ::djinni::JniClass<NativeExceptionLoggerInterface>;
    friend ::djinni::JniInterface<::ExceptionLoggerInterface, NativeExceptionLoggerInterface>;

};

} // namespace djinni_generated
