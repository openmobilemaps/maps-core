// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#pragma once

#include "ErrorManager.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeErrorManager final : ::djinni::JniInterface<::ErrorManager, NativeErrorManager> {
public:
    using CppType = std::shared_ptr<::ErrorManager>;
    using CppOptType = std::shared_ptr<::ErrorManager>;
    using JniType = jobject;

    using Boxed = NativeErrorManager;

    ~NativeErrorManager();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeErrorManager>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeErrorManager>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeErrorManager();
    friend ::djinni::JniClass<NativeErrorManager>;
    friend ::djinni::JniInterface<::ErrorManager, NativeErrorManager>;

};

} // namespace djinni_generated
