// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#pragma once

#include "ComputeObjectInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeComputeObjectInterface final : ::djinni::JniInterface<::ComputeObjectInterface, NativeComputeObjectInterface> {
public:
    using CppType = std::shared_ptr<::ComputeObjectInterface>;
    using CppOptType = std::shared_ptr<::ComputeObjectInterface>;
    using JniType = jobject;

    using Boxed = NativeComputeObjectInterface;

    ~NativeComputeObjectInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeComputeObjectInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeComputeObjectInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeComputeObjectInterface();
    friend ::djinni::JniClass<NativeComputeObjectInterface>;
    friend ::djinni::JniInterface<::ComputeObjectInterface, NativeComputeObjectInterface>;

    class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::ComputeObjectInterface
    {
    public:
        JavaProxy(JniType j);
        ~JavaProxy();

        void compute(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & context, int64_t vpMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) override;

    private:
        friend ::djinni::JniInterface<::ComputeObjectInterface, ::djinni_generated::NativeComputeObjectInterface>;
    };

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/graphics/objects/ComputeObjectInterface") };
    const jmethodID method_compute { ::djinni::jniGetMethodID(clazz.get(), "compute", "(Lio/openmobilemaps/mapscore/shared/graphics/RenderingContextInterface;JLio/openmobilemaps/mapscore/shared/graphics/common/Vec3D;D)V") };
};

} // namespace djinni_generated
