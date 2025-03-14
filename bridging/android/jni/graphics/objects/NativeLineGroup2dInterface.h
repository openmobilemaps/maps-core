// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#pragma once

#include "LineGroup2dInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeLineGroup2dInterface final : ::djinni::JniInterface<::LineGroup2dInterface, NativeLineGroup2dInterface> {
public:
    using CppType = std::shared_ptr<::LineGroup2dInterface>;
    using CppOptType = std::shared_ptr<::LineGroup2dInterface>;
    using JniType = jobject;

    using Boxed = NativeLineGroup2dInterface;

    ~NativeLineGroup2dInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeLineGroup2dInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeLineGroup2dInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeLineGroup2dInterface();
    friend ::djinni::JniClass<NativeLineGroup2dInterface>;
    friend ::djinni::JniInterface<::LineGroup2dInterface, NativeLineGroup2dInterface>;

    class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::LineGroup2dInterface
    {
    public:
        JavaProxy(JniType j);
        ~JavaProxy();

        void setLines(const ::SharedBytes & lines, const ::SharedBytes & indices, const ::Vec3D & origin, bool is3d) override;
        /*not-null*/ std::shared_ptr<::GraphicsObjectInterface> asGraphicsObject() override;

    private:
        friend ::djinni::JniInterface<::LineGroup2dInterface, ::djinni_generated::NativeLineGroup2dInterface>;
    };

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/graphics/objects/LineGroup2dInterface") };
    const jmethodID method_setLines { ::djinni::jniGetMethodID(clazz.get(), "setLines", "(Lio/openmobilemaps/mapscore/shared/graphics/common/SharedBytes;Lio/openmobilemaps/mapscore/shared/graphics/common/SharedBytes;Lio/openmobilemaps/mapscore/shared/graphics/common/Vec3D;Z)V") };
    const jmethodID method_asGraphicsObject { ::djinni::jniGetMethodID(clazz.get(), "asGraphicsObject", "()Lio/openmobilemaps/mapscore/shared/graphics/objects/GraphicsObjectInterface;") };
};

} // namespace djinni_generated
