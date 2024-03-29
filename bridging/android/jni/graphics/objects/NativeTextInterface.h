// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#pragma once

#include "TextInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeTextInterface final : ::djinni::JniInterface<::TextInterface, NativeTextInterface> {
public:
    using CppType = std::shared_ptr<::TextInterface>;
    using CppOptType = std::shared_ptr<::TextInterface>;
    using JniType = jobject;

    using Boxed = NativeTextInterface;

    ~NativeTextInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeTextInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeTextInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeTextInterface();
    friend ::djinni::JniClass<NativeTextInterface>;
    friend ::djinni::JniInterface<::TextInterface, NativeTextInterface>;

    class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::TextInterface
    {
    public:
        JavaProxy(JniType j);
        ~JavaProxy();

        void setTextsShared(const ::SharedBytes & vertices, const ::SharedBytes & indices) override;
        void loadTexture(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & context, const /*not-null*/ std::shared_ptr<::TextureHolderInterface> & textureHolder) override;
        void removeTexture() override;
        /*not-null*/ std::shared_ptr<::GraphicsObjectInterface> asGraphicsObject() override;

    private:
        friend ::djinni::JniInterface<::TextInterface, ::djinni_generated::NativeTextInterface>;
    };

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/graphics/objects/TextInterface") };
    const jmethodID method_setTextsShared { ::djinni::jniGetMethodID(clazz.get(), "setTextsShared", "(Lio/openmobilemaps/mapscore/shared/graphics/common/SharedBytes;Lio/openmobilemaps/mapscore/shared/graphics/common/SharedBytes;)V") };
    const jmethodID method_loadTexture { ::djinni::jniGetMethodID(clazz.get(), "loadTexture", "(Lio/openmobilemaps/mapscore/shared/graphics/RenderingContextInterface;Lio/openmobilemaps/mapscore/shared/graphics/objects/TextureHolderInterface;)V") };
    const jmethodID method_removeTexture { ::djinni::jniGetMethodID(clazz.get(), "removeTexture", "()V") };
    const jmethodID method_asGraphicsObject { ::djinni::jniGetMethodID(clazz.get(), "asGraphicsObject", "()Lio/openmobilemaps/mapscore/shared/graphics/objects/GraphicsObjectInterface;") };
};

} // namespace djinni_generated
