// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativeTextInterface.h"  // my header
#include "NativeGraphicsObjectInterface.h"
#include "NativeRenderingContextInterface.h"
#include "NativeSharedBytes.h"
#include "NativeTextureHolderInterface.h"

namespace djinni_generated {

NativeTextInterface::NativeTextInterface() : ::djinni::JniInterface<::TextInterface, NativeTextInterface>("io/openmobilemaps/mapscore/shared/graphics/objects/TextInterface$CppProxy") {}

NativeTextInterface::~NativeTextInterface() = default;

NativeTextInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeTextInterface::JavaProxy::~JavaProxy() = default;

void NativeTextInterface::JavaProxy::setTextsShared(const ::SharedBytes & c_vertices, const ::SharedBytes & c_indices) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTextInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_setTextsShared,
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_vertices)),
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_indices)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeTextInterface::JavaProxy::loadTexture(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_context, const /*not-null*/ std::shared_ptr<::TextureHolderInterface> & c_textureHolder) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTextInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_loadTexture,
                           ::djinni::get(::djinni_generated::NativeRenderingContextInterface::fromCpp(jniEnv, c_context)),
                           ::djinni::get(::djinni_generated::NativeTextureHolderInterface::fromCpp(jniEnv, c_textureHolder)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeTextInterface::JavaProxy::removeTexture() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTextInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_removeTexture);
    ::djinni::jniExceptionCheck(jniEnv);
}
/*not-null*/ std::shared_ptr<::GraphicsObjectInterface> NativeTextInterface::JavaProxy::asGraphicsObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTextInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_asGraphicsObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeGraphicsObjectInterface::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_TextInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::TextInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_TextInterface_00024CppProxy_native_1setTextsShared(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeSharedBytes::JniType j_vertices, ::djinni_generated::NativeSharedBytes::JniType j_indices)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TextInterface>(nativeRef);
        ref->setTextsShared(::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_vertices),
                            ::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_indices));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_TextInterface_00024CppProxy_native_1loadTexture(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeRenderingContextInterface::JniType j_context, jobject j_textureHolder)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TextInterface>(nativeRef);
        ref->loadTexture(::djinni_generated::NativeRenderingContextInterface::toCpp(jniEnv, j_context),
                         ::djinni_generated::NativeTextureHolderInterface::toCpp(jniEnv, j_textureHolder));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_TextInterface_00024CppProxy_native_1removeTexture(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TextInterface>(nativeRef);
        ref->removeTexture();
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_TextInterface_00024CppProxy_native_1asGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TextInterface>(nativeRef);
        auto r = ref->asGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
