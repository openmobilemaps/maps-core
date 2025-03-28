// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#include "NativeRenderPassInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeMaskingObjectInterface.h"
#include "NativeRectI.h"
#include "NativeRenderObjectInterface.h"
#include "NativeRenderPassConfig.h"
#include "NativeRenderTargetInterface.h"

namespace djinni_generated {

NativeRenderPassInterface::NativeRenderPassInterface() : ::djinni::JniInterface<::RenderPassInterface, NativeRenderPassInterface>("io/openmobilemaps/mapscore/shared/graphics/RenderPassInterface$CppProxy") {}

NativeRenderPassInterface::~NativeRenderPassInterface() = default;

NativeRenderPassInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeRenderPassInterface::JavaProxy::~JavaProxy() = default;

std::vector</*not-null*/ std::shared_ptr<::RenderObjectInterface>> NativeRenderPassInterface::JavaProxy::getRenderObjects() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderPassInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getRenderObjects);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::List<::djinni_generated::NativeRenderObjectInterface>::toCpp(jniEnv, jret);
}
void NativeRenderPassInterface::JavaProxy::addRenderObject(const /*not-null*/ std::shared_ptr<::RenderObjectInterface> & c_renderObject) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderPassInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_addRenderObject,
                           ::djinni::get(::djinni_generated::NativeRenderObjectInterface::fromCpp(jniEnv, c_renderObject)));
    ::djinni::jniExceptionCheck(jniEnv);
}
::RenderPassConfig NativeRenderPassInterface::JavaProxy::getRenderPassConfig() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderPassInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getRenderPassConfig);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeRenderPassConfig::toCpp(jniEnv, jret);
}
/*nullable*/ std::shared_ptr<::RenderTargetInterface> NativeRenderPassInterface::JavaProxy::getRenderTargetInterface() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderPassInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getRenderTargetInterface);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::Optional<std::optional, ::djinni_generated::NativeRenderTargetInterface>::toCpp(jniEnv, jret);
}
/*nullable*/ std::shared_ptr<::MaskingObjectInterface> NativeRenderPassInterface::JavaProxy::getMaskingObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderPassInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getMaskingObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::Optional<std::optional, ::djinni_generated::NativeMaskingObjectInterface>::toCpp(jniEnv, jret);
}
std::optional<::RectI> NativeRenderPassInterface::JavaProxy::getScissoringRect() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderPassInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getScissoringRect);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::Optional<std::optional, ::djinni_generated::NativeRectI>::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::RenderPassInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_native_1getRenderObjects(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderPassInterface>(nativeRef);
        auto r = ref->getRenderObjects();
        return ::djinni::release(::djinni::List<::djinni_generated::NativeRenderObjectInterface>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_native_1addRenderObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_renderObject)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderPassInterface>(nativeRef);
        ref->addRenderObject(::djinni_generated::NativeRenderObjectInterface::toCpp(jniEnv, j_renderObject));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_native_1getRenderPassConfig(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderPassInterface>(nativeRef);
        auto r = ref->getRenderPassConfig();
        return ::djinni::release(::djinni_generated::NativeRenderPassConfig::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_native_1getRenderTargetInterface(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderPassInterface>(nativeRef);
        auto r = ref->getRenderTargetInterface();
        return ::djinni::release(::djinni::Optional<std::optional, ::djinni_generated::NativeRenderTargetInterface>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT ::djinni_generated::NativeMaskingObjectInterface::Boxed::JniType JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_native_1getMaskingObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderPassInterface>(nativeRef);
        auto r = ref->getMaskingObject();
        return ::djinni::release(::djinni::Optional<std::optional, ::djinni_generated::NativeMaskingObjectInterface>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT ::djinni_generated::NativeRectI::Boxed::JniType JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderPassInterface_00024CppProxy_native_1getScissoringRect(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderPassInterface>(nativeRef);
        auto r = ref->getScissoringRect();
        return ::djinni::release(::djinni::Optional<std::optional, ::djinni_generated::NativeRectI>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
