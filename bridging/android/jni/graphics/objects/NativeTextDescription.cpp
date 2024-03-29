// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativeTextDescription.h"  // my header
#include "Marshal.hpp"
#include "NativeGlyphDescription.h"

namespace djinni_generated {

NativeTextDescription::NativeTextDescription() = default;

NativeTextDescription::~NativeTextDescription() = default;

auto NativeTextDescription::fromCpp(JNIEnv* jniEnv, const CppType& c) -> ::djinni::LocalRef<JniType> {
    const auto& data = ::djinni::JniClass<NativeTextDescription>::get();
    auto r = ::djinni::LocalRef<JniType>{jniEnv->NewObject(data.clazz.get(), data.jconstructor,
                                                           ::djinni::get(::djinni::List<::djinni_generated::NativeGlyphDescription>::fromCpp(jniEnv, c.glyphs)))};
    ::djinni::jniExceptionCheck(jniEnv);
    return r;
}

auto NativeTextDescription::toCpp(JNIEnv* jniEnv, JniType j) -> CppType {
    ::djinni::JniLocalScope jscope(jniEnv, 2);
    assert(j != nullptr);
    const auto& data = ::djinni::JniClass<NativeTextDescription>::get();
    return {::djinni::List<::djinni_generated::NativeGlyphDescription>::toCpp(jniEnv, jniEnv->GetObjectField(j, data.field_glyphs))};
}

} // namespace djinni_generated
