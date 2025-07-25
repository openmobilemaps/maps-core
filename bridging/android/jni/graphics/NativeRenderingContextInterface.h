// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include "RenderingContextInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeRenderingContextInterface final : ::djinni::JniInterface<::RenderingContextInterface, NativeRenderingContextInterface> {
public:
    using CppType = std::shared_ptr<::RenderingContextInterface>;
    using CppOptType = std::shared_ptr<::RenderingContextInterface>;
    using JniType = jobject;

    using Boxed = NativeRenderingContextInterface;

    ~NativeRenderingContextInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeRenderingContextInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeRenderingContextInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeRenderingContextInterface();
    friend ::djinni::JniClass<NativeRenderingContextInterface>;
    friend ::djinni::JniInterface<::RenderingContextInterface, NativeRenderingContextInterface>;

    class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::RenderingContextInterface
    {
    public:
        JavaProxy(JniType j);
        ~JavaProxy();

        void onSurfaceCreated() override;
        void setViewportSize(const ::Vec2I & size) override;
        ::Vec2I getViewportSize() override;
        void setBackgroundColor(const ::Color & color) override;
        void setCulling(::RenderingCullMode mode) override;
        void setupDrawFrame(int64_t vpMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) override;
        void preRenderStencilMask() override;
        void postRenderStencilMask() override;
        void applyScissorRect(const std::optional<::RectI> & scissorRect) override;
        /*nullable*/ std::shared_ptr<::OpenGlRenderingContextInterface> asOpenGlRenderingContext() override;

    private:
        friend ::djinni::JniInterface<::RenderingContextInterface, ::djinni_generated::NativeRenderingContextInterface>;
    };

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/graphics/RenderingContextInterface") };
    const jmethodID method_onSurfaceCreated { ::djinni::jniGetMethodID(clazz.get(), "onSurfaceCreated", "()V") };
    const jmethodID method_setViewportSize { ::djinni::jniGetMethodID(clazz.get(), "setViewportSize", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2I;)V") };
    const jmethodID method_getViewportSize { ::djinni::jniGetMethodID(clazz.get(), "getViewportSize", "()Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2I;") };
    const jmethodID method_setBackgroundColor { ::djinni::jniGetMethodID(clazz.get(), "setBackgroundColor", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Color;)V") };
    const jmethodID method_setCulling { ::djinni::jniGetMethodID(clazz.get(), "setCulling", "(Lio/openmobilemaps/mapscore/shared/graphics/RenderingCullMode;)V") };
    const jmethodID method_setupDrawFrame { ::djinni::jniGetMethodID(clazz.get(), "setupDrawFrame", "(JLio/openmobilemaps/mapscore/shared/graphics/common/Vec3D;D)V") };
    const jmethodID method_preRenderStencilMask { ::djinni::jniGetMethodID(clazz.get(), "preRenderStencilMask", "()V") };
    const jmethodID method_postRenderStencilMask { ::djinni::jniGetMethodID(clazz.get(), "postRenderStencilMask", "()V") };
    const jmethodID method_applyScissorRect { ::djinni::jniGetMethodID(clazz.get(), "applyScissorRect", "(Lio/openmobilemaps/mapscore/shared/graphics/common/RectI;)V") };
    const jmethodID method_asOpenGlRenderingContext { ::djinni::jniGetMethodID(clazz.get(), "asOpenGlRenderingContext", "()Lio/openmobilemaps/mapscore/shared/graphics/OpenGlRenderingContextInterface;") };
};

} // namespace djinni_generated
