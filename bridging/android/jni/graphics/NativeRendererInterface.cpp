// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#include "NativeRendererInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeCameraInterface.h"
#include "NativeComputePassInterface.h"
#include "NativeRenderPassInterface.h"
#include "NativeRenderTargetInterface.h"
#include "NativeRenderingContextInterface.h"

namespace djinni_generated {

NativeRendererInterface::NativeRendererInterface() : ::djinni::JniInterface<::RendererInterface, NativeRendererInterface>("io/openmobilemaps/mapscore/shared/graphics/RendererInterface$CppProxy") {}

NativeRendererInterface::~NativeRendererInterface() = default;

NativeRendererInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeRendererInterface::JavaProxy::~JavaProxy() = default;

void NativeRendererInterface::JavaProxy::addToRenderQueue(const /*not-null*/ std::shared_ptr<::RenderPassInterface> & c_renderPass) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRendererInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_addToRenderQueue,
                           ::djinni::get(::djinni_generated::NativeRenderPassInterface::fromCpp(jniEnv, c_renderPass)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeRendererInterface::JavaProxy::addToComputeQueue(const /*not-null*/ std::shared_ptr<::ComputePassInterface> & c_computePass) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRendererInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_addToComputeQueue,
                           ::djinni::get(::djinni_generated::NativeComputePassInterface::fromCpp(jniEnv, c_computePass)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeRendererInterface::JavaProxy::drawFrame(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_renderingContext, const /*not-null*/ std::shared_ptr<::CameraInterface> & c_camera, const /*nullable*/ std::shared_ptr<::RenderTargetInterface> & c_target) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRendererInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_drawFrame,
                           ::djinni::get(::djinni_generated::NativeRenderingContextInterface::fromCpp(jniEnv, c_renderingContext)),
                           ::djinni::get(::djinni_generated::NativeCameraInterface::fromCpp(jniEnv, c_camera)),
                           ::djinni::get(::djinni::Optional<std::optional, ::djinni_generated::NativeRenderTargetInterface>::fromCpp(jniEnv, c_target)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeRendererInterface::JavaProxy::compute(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_renderingContext, const /*not-null*/ std::shared_ptr<::CameraInterface> & c_camera) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeRendererInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_compute,
                           ::djinni::get(::djinni_generated::NativeRenderingContextInterface::fromCpp(jniEnv, c_renderingContext)),
                           ::djinni::get(::djinni_generated::NativeCameraInterface::fromCpp(jniEnv, c_camera)));
    ::djinni::jniExceptionCheck(jniEnv);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RendererInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::RendererInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RendererInterface_00024CppProxy_native_1addToRenderQueue(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_renderPass)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RendererInterface>(nativeRef);
        ref->addToRenderQueue(::djinni_generated::NativeRenderPassInterface::toCpp(jniEnv, j_renderPass));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RendererInterface_00024CppProxy_native_1addToComputeQueue(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_computePass)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RendererInterface>(nativeRef);
        ref->addToComputeQueue(::djinni_generated::NativeComputePassInterface::toCpp(jniEnv, j_computePass));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RendererInterface_00024CppProxy_native_1drawFrame(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_renderingContext, jobject j_camera, jobject j_target)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RendererInterface>(nativeRef);
        ref->drawFrame(::djinni_generated::NativeRenderingContextInterface::toCpp(jniEnv, j_renderingContext),
                       ::djinni_generated::NativeCameraInterface::toCpp(jniEnv, j_camera),
                       ::djinni::Optional<std::optional, ::djinni_generated::NativeRenderTargetInterface>::toCpp(jniEnv, j_target));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RendererInterface_00024CppProxy_native_1compute(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_renderingContext, jobject j_camera)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RendererInterface>(nativeRef);
        ref->compute(::djinni_generated::NativeRenderingContextInterface::toCpp(jniEnv, j_renderingContext),
                     ::djinni_generated::NativeCameraInterface::toCpp(jniEnv, j_camera));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

} // namespace djinni_generated
