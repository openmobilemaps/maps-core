// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include "OpenGlRenderTargetInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeOpenGlRenderTargetInterface final : ::djinni::JniInterface<::OpenGlRenderTargetInterface, NativeOpenGlRenderTargetInterface> {
public:
    using CppType = std::shared_ptr<::OpenGlRenderTargetInterface>;
    using CppOptType = std::shared_ptr<::OpenGlRenderTargetInterface>;
    using JniType = jobject;

    using Boxed = NativeOpenGlRenderTargetInterface;

    ~NativeOpenGlRenderTargetInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeOpenGlRenderTargetInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeOpenGlRenderTargetInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeOpenGlRenderTargetInterface();
    friend ::djinni::JniClass<NativeOpenGlRenderTargetInterface>;
    friend ::djinni::JniInterface<::OpenGlRenderTargetInterface, NativeOpenGlRenderTargetInterface>;

    class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::OpenGlRenderTargetInterface
    {
    public:
        JavaProxy(JniType j);
        ~JavaProxy();

        /*not-null*/ std::shared_ptr<::RenderTargetInterface> asRenderTargetInterface() override;
        void setup(const ::Vec2I & size) override;
        void clear() override;
        void bindFramebuffer(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & renderingContext) override;
        void unbindFramebuffer() override;
        int32_t getTextureId() override;

    private:
        friend ::djinni::JniInterface<::OpenGlRenderTargetInterface, ::djinni_generated::NativeOpenGlRenderTargetInterface>;
    };

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/graphics/OpenGlRenderTargetInterface") };
    const jmethodID method_asRenderTargetInterface { ::djinni::jniGetMethodID(clazz.get(), "asRenderTargetInterface", "()Lio/openmobilemaps/mapscore/shared/graphics/RenderTargetInterface;") };
    const jmethodID method_setup { ::djinni::jniGetMethodID(clazz.get(), "setup", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2I;)V") };
    const jmethodID method_clear { ::djinni::jniGetMethodID(clazz.get(), "clear", "()V") };
    const jmethodID method_bindFramebuffer { ::djinni::jniGetMethodID(clazz.get(), "bindFramebuffer", "(Lio/openmobilemaps/mapscore/shared/graphics/RenderingContextInterface;)V") };
    const jmethodID method_unbindFramebuffer { ::djinni::jniGetMethodID(clazz.get(), "unbindFramebuffer", "()V") };
    const jmethodID method_getTextureId { ::djinni::jniGetMethodID(clazz.get(), "getTextureId", "()I") };
};

} // namespace djinni_generated
