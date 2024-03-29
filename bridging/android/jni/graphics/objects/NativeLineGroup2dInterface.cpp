// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativeLineGroup2dInterface.h"  // my header
#include "NativeGraphicsObjectInterface.h"
#include "NativeSharedBytes.h"

namespace djinni_generated {

NativeLineGroup2dInterface::NativeLineGroup2dInterface() : ::djinni::JniInterface<::LineGroup2dInterface, NativeLineGroup2dInterface>("io/openmobilemaps/mapscore/shared/graphics/objects/LineGroup2dInterface$CppProxy") {}

NativeLineGroup2dInterface::~NativeLineGroup2dInterface() = default;

NativeLineGroup2dInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeLineGroup2dInterface::JavaProxy::~JavaProxy() = default;

void NativeLineGroup2dInterface::JavaProxy::setLines(const ::SharedBytes & c_lines, const ::SharedBytes & c_indices) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeLineGroup2dInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_setLines,
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_lines)),
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_indices)));
    ::djinni::jniExceptionCheck(jniEnv);
}
/*not-null*/ std::shared_ptr<::GraphicsObjectInterface> NativeLineGroup2dInterface::JavaProxy::asGraphicsObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeLineGroup2dInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_asGraphicsObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeGraphicsObjectInterface::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_LineGroup2dInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::LineGroup2dInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_LineGroup2dInterface_00024CppProxy_native_1setLines(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeSharedBytes::JniType j_lines, ::djinni_generated::NativeSharedBytes::JniType j_indices)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::LineGroup2dInterface>(nativeRef);
        ref->setLines(::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_lines),
                      ::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_indices));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_LineGroup2dInterface_00024CppProxy_native_1asGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::LineGroup2dInterface>(nativeRef);
        auto r = ref->asGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
