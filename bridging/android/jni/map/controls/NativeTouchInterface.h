// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from touch_handler.djinni

#pragma once

#include "TouchInterface.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeTouchInterface final : ::djinni::JniInterface<::TouchInterface, NativeTouchInterface> {
public:
    using CppType = std::shared_ptr<::TouchInterface>;
    using CppOptType = std::shared_ptr<::TouchInterface>;
    using JniType = jobject;

    using Boxed = NativeTouchInterface;

    ~NativeTouchInterface();

    static CppType toCpp(JNIEnv* jniEnv, JniType j) { return ::djinni::JniClass<NativeTouchInterface>::get()._fromJava(jniEnv, j); }
    static ::djinni::LocalRef<JniType> fromCppOpt(JNIEnv* jniEnv, const CppOptType& c) { return {jniEnv, ::djinni::JniClass<NativeTouchInterface>::get()._toJava(jniEnv, c)}; }
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c) { return fromCppOpt(jniEnv, c); }

private:
    NativeTouchInterface();
    friend ::djinni::JniClass<NativeTouchInterface>;
    friend ::djinni::JniInterface<::TouchInterface, NativeTouchInterface>;

    class JavaProxy final : ::djinni::JavaProxyHandle<JavaProxy>, public ::TouchInterface
    {
    public:
        JavaProxy(JniType j);
        ~JavaProxy();

        bool onTouchDown(const ::Vec2F & posScreen) override;
        bool onClickUnconfirmed(const ::Vec2F & posScreen) override;
        bool onClickConfirmed(const ::Vec2F & posScreen) override;
        bool onDoubleClick(const ::Vec2F & posScreen) override;
        bool onLongPress(const ::Vec2F & posScreen) override;
        bool onMove(const ::Vec2F & deltaScreen, bool confirmed, bool doubleClick) override;
        bool onMoveComplete() override;
        bool onOneFingerDoubleClickMoveComplete() override;
        bool onTwoFingerClick(const ::Vec2F & posScreen1, const ::Vec2F & posScreen2) override;
        bool onTwoFingerMove(const std::vector<::Vec2F> & posScreenOld, const std::vector<::Vec2F> & posScreenNew) override;
        bool onTwoFingerMoveComplete() override;
        void clearTouch() override;

    private:
        friend ::djinni::JniInterface<::TouchInterface, ::djinni_generated::NativeTouchInterface>;
    };

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/map/controls/TouchInterface") };
    const jmethodID method_onTouchDown { ::djinni::jniGetMethodID(clazz.get(), "onTouchDown", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;)Z") };
    const jmethodID method_onClickUnconfirmed { ::djinni::jniGetMethodID(clazz.get(), "onClickUnconfirmed", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;)Z") };
    const jmethodID method_onClickConfirmed { ::djinni::jniGetMethodID(clazz.get(), "onClickConfirmed", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;)Z") };
    const jmethodID method_onDoubleClick { ::djinni::jniGetMethodID(clazz.get(), "onDoubleClick", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;)Z") };
    const jmethodID method_onLongPress { ::djinni::jniGetMethodID(clazz.get(), "onLongPress", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;)Z") };
    const jmethodID method_onMove { ::djinni::jniGetMethodID(clazz.get(), "onMove", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;ZZ)Z") };
    const jmethodID method_onMoveComplete { ::djinni::jniGetMethodID(clazz.get(), "onMoveComplete", "()Z") };
    const jmethodID method_onOneFingerDoubleClickMoveComplete { ::djinni::jniGetMethodID(clazz.get(), "onOneFingerDoubleClickMoveComplete", "()Z") };
    const jmethodID method_onTwoFingerClick { ::djinni::jniGetMethodID(clazz.get(), "onTwoFingerClick", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2F;)Z") };
    const jmethodID method_onTwoFingerMove { ::djinni::jniGetMethodID(clazz.get(), "onTwoFingerMove", "(Ljava/util/ArrayList;Ljava/util/ArrayList;)Z") };
    const jmethodID method_onTwoFingerMoveComplete { ::djinni::jniGetMethodID(clazz.get(), "onTwoFingerMoveComplete", "()Z") };
    const jmethodID method_clearTouch { ::djinni::jniGetMethodID(clazz.get(), "clearTouch", "()V") };
};

} // namespace djinni_generated
