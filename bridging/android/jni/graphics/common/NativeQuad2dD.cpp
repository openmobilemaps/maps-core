// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#include "NativeQuad2dD.h"  // my header
#include "NativeVec2D.h"

namespace djinni_generated {

NativeQuad2dD::NativeQuad2dD() = default;

NativeQuad2dD::~NativeQuad2dD() = default;

auto NativeQuad2dD::fromCpp(JNIEnv* jniEnv, const CppType& c) -> ::djinni::LocalRef<JniType> {
    const auto& data = ::djinni::JniClass<NativeQuad2dD>::get();
    auto r = ::djinni::LocalRef<JniType>{jniEnv->NewObject(data.clazz.get(), data.jconstructor,
                                                           ::djinni::get(::djinni_generated::NativeVec2D::fromCpp(jniEnv, c.topLeft)),
                                                           ::djinni::get(::djinni_generated::NativeVec2D::fromCpp(jniEnv, c.topRight)),
                                                           ::djinni::get(::djinni_generated::NativeVec2D::fromCpp(jniEnv, c.bottomRight)),
                                                           ::djinni::get(::djinni_generated::NativeVec2D::fromCpp(jniEnv, c.bottomLeft)))};
    ::djinni::jniExceptionCheck(jniEnv);
    return r;
}

auto NativeQuad2dD::toCpp(JNIEnv* jniEnv, JniType j) -> CppType {
    ::djinni::JniLocalScope jscope(jniEnv, 5);
    assert(j != nullptr);
    const auto& data = ::djinni::JniClass<NativeQuad2dD>::get();
    return {::djinni_generated::NativeVec2D::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_topLeft)),
            ::djinni_generated::NativeVec2D::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_topRight)),
            ::djinni_generated::NativeVec2D::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_bottomRight)),
            ::djinni_generated::NativeVec2D::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_bottomLeft))};
}

} // namespace djinni_generated
