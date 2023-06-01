// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#include "NativeRasterShaderStyle.h"  // my header
#include "Marshal.hpp"

namespace djinni_generated {

NativeRasterShaderStyle::NativeRasterShaderStyle() = default;

NativeRasterShaderStyle::~NativeRasterShaderStyle() = default;

auto NativeRasterShaderStyle::fromCpp(JNIEnv* jniEnv, const CppType& c) -> ::djinni::LocalRef<JniType> {
    const auto& data = ::djinni::JniClass<NativeRasterShaderStyle>::get();
    auto r = ::djinni::LocalRef<JniType>{jniEnv->NewObject(data.clazz.get(), data.jconstructor,
                                                           ::djinni::get(::djinni::F32::fromCpp(jniEnv, c.opacity)),
                                                           ::djinni::get(::djinni::F32::fromCpp(jniEnv, c.brightnessMin)),
                                                           ::djinni::get(::djinni::F32::fromCpp(jniEnv, c.brightnessMax)),
                                                           ::djinni::get(::djinni::F32::fromCpp(jniEnv, c.contrast)),
                                                           ::djinni::get(::djinni::F32::fromCpp(jniEnv, c.saturation)))};
    ::djinni::jniExceptionCheck(jniEnv);
    return r;
}

auto NativeRasterShaderStyle::toCpp(JNIEnv* jniEnv, JniType j) -> CppType {
    ::djinni::JniLocalScope jscope(jniEnv, 6);
    assert(j != nullptr);
    const auto& data = ::djinni::JniClass<NativeRasterShaderStyle>::get();
    return {::djinni::F32::toCpp(jniEnv, jniEnv->GetFloatField(j, data.field_opacity)),
            ::djinni::F32::toCpp(jniEnv, jniEnv->GetFloatField(j, data.field_brightnessMin)),
            ::djinni::F32::toCpp(jniEnv, jniEnv->GetFloatField(j, data.field_brightnessMax)),
            ::djinni::F32::toCpp(jniEnv, jniEnv->GetFloatField(j, data.field_contrast)),
            ::djinni::F32::toCpp(jniEnv, jniEnv->GetFloatField(j, data.field_saturation))};
}

} // namespace djinni_generated