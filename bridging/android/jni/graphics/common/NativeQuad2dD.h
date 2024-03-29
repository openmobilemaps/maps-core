// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#pragma once

#include "Quad2dD.h"
#include "djinni_support.hpp"

namespace djinni_generated {

class NativeQuad2dD final {
public:
    using CppType = ::Quad2dD;
    using JniType = jobject;

    using Boxed = NativeQuad2dD;

    ~NativeQuad2dD();

    static CppType toCpp(JNIEnv* jniEnv, JniType j);
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c);

private:
    NativeQuad2dD();
    friend ::djinni::JniClass<NativeQuad2dD>;

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("io/openmobilemaps/mapscore/shared/graphics/common/Quad2dD") };
    const jmethodID jconstructor { ::djinni::jniGetMethodID(clazz.get(), "<init>", "(Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;)V") };
    const jfieldID field_topLeft { ::djinni::jniGetFieldID(clazz.get(), "topLeft", "Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;") };
    const jfieldID field_topRight { ::djinni::jniGetFieldID(clazz.get(), "topRight", "Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;") };
    const jfieldID field_bottomRight { ::djinni::jniGetFieldID(clazz.get(), "bottomRight", "Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;") };
    const jfieldID field_bottomLeft { ::djinni::jniGetFieldID(clazz.get(), "bottomLeft", "Lio/openmobilemaps/mapscore/shared/graphics/common/Vec2D;") };
};

} // namespace djinni_generated
