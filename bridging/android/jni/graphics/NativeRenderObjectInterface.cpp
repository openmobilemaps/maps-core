// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#include "NativeRenderObjectInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeGraphicsObjectInterface.h"

namespace djinni_generated {

NativeRenderObjectInterface::NativeRenderObjectInterface() : ::djinni::JniInterface<::RenderObjectInterface, NativeRenderObjectInterface>("io/openmobilemaps/mapscore/shared/graphics/RenderObjectInterface$CppProxy") {}

NativeRenderObjectInterface::~NativeRenderObjectInterface() = default;

NativeRenderObjectInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeRenderObjectInterface::JavaProxy::~JavaProxy() = default;

/*not-null*/ std::shared_ptr<::GraphicsObjectInterface> NativeRenderObjectInterface::JavaProxy::getGraphicsObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderObjectInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getGraphicsObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeGraphicsObjectInterface::toCpp(jniEnv, jret);
}
bool NativeRenderObjectInterface::JavaProxy::hasCustomModelMatrix() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderObjectInterface>::get();
    auto jret = jniEnv->CallBooleanMethod(Handle::get().get(), data.method_hasCustomModelMatrix);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::Bool::toCpp(jniEnv, jret);
}
bool NativeRenderObjectInterface::JavaProxy::isScreenSpaceCoords() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderObjectInterface>::get();
    auto jret = jniEnv->CallBooleanMethod(Handle::get().get(), data.method_isScreenSpaceCoords);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::Bool::toCpp(jniEnv, jret);
}
std::vector<float> NativeRenderObjectInterface::JavaProxy::getCustomModelMatrix() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderObjectInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_getCustomModelMatrix);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::List<::djinni::F32>::toCpp(jniEnv, jret);
}
void NativeRenderObjectInterface::JavaProxy::setHidden(bool c_hidden) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderObjectInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_setHidden,
                           ::djinni::get(::djinni::Bool::fromCpp(jniEnv, c_hidden)));
    ::djinni::jniExceptionCheck(jniEnv);
}
bool NativeRenderObjectInterface::JavaProxy::isHidden() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRenderObjectInterface>::get();
    auto jret = jniEnv->CallBooleanMethod(Handle::get().get(), data.method_isHidden);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::Bool::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::RenderObjectInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT ::djinni_generated::NativeGraphicsObjectInterface::JniType JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_native_1getGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderObjectInterface>(nativeRef);
        auto r = ref->getGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jboolean JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_native_1hasCustomModelMatrix(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderObjectInterface>(nativeRef);
        auto r = ref->hasCustomModelMatrix();
        return ::djinni::release(::djinni::Bool::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jboolean JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_native_1isScreenSpaceCoords(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderObjectInterface>(nativeRef);
        auto r = ref->isScreenSpaceCoords();
        return ::djinni::release(::djinni::Bool::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_native_1getCustomModelMatrix(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderObjectInterface>(nativeRef);
        auto r = ref->getCustomModelMatrix();
        return ::djinni::release(::djinni::List<::djinni::F32>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_native_1setHidden(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jboolean j_hidden)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderObjectInterface>(nativeRef);
        ref->setHidden(::djinni::Bool::toCpp(jniEnv, j_hidden));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jboolean JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RenderObjectInterface_00024CppProxy_native_1isHidden(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderObjectInterface>(nativeRef);
        auto r = ref->isHidden();
        return ::djinni::release(::djinni::Bool::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
