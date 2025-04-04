// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativeMaskingObjectInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeGraphicsObjectInterface.h"
#include "NativeRenderPassConfig.h"
#include "NativeRenderingContextInterface.h"
#include "NativeVec3D.h"

namespace djinni_generated {

NativeMaskingObjectInterface::NativeMaskingObjectInterface() : ::djinni::JniInterface<::MaskingObjectInterface, NativeMaskingObjectInterface>("io/openmobilemaps/mapscore/shared/graphics/objects/MaskingObjectInterface$CppProxy") {}

NativeMaskingObjectInterface::~NativeMaskingObjectInterface() = default;

NativeMaskingObjectInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeMaskingObjectInterface::JavaProxy::~JavaProxy() = default;

/*not-null*/ std::shared_ptr<::GraphicsObjectInterface> NativeMaskingObjectInterface::JavaProxy::asGraphicsObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeMaskingObjectInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_asGraphicsObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeGraphicsObjectInterface::toCpp(jniEnv, jret);
}
void NativeMaskingObjectInterface::JavaProxy::renderAsMask(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_context, const ::RenderPassConfig & c_renderPass, int64_t c_vpMatrix, int64_t c_mMatrix, const ::Vec3D & c_origin, double c_screenPixelAsRealMeterFactor) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeMaskingObjectInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_renderAsMask,
                           ::djinni::get(::djinni_generated::NativeRenderingContextInterface::fromCpp(jniEnv, c_context)),
                           ::djinni::get(::djinni_generated::NativeRenderPassConfig::fromCpp(jniEnv, c_renderPass)),
                           ::djinni::get(::djinni::I64::fromCpp(jniEnv, c_vpMatrix)),
                           ::djinni::get(::djinni::I64::fromCpp(jniEnv, c_mMatrix)),
                           ::djinni::get(::djinni_generated::NativeVec3D::fromCpp(jniEnv, c_origin)),
                           ::djinni::get(::djinni::F64::fromCpp(jniEnv, c_screenPixelAsRealMeterFactor)));
    ::djinni::jniExceptionCheck(jniEnv);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_MaskingObjectInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::MaskingObjectInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_MaskingObjectInterface_00024CppProxy_native_1asGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::MaskingObjectInterface>(nativeRef);
        auto r = ref->asGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_MaskingObjectInterface_00024CppProxy_native_1renderAsMask(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeRenderingContextInterface::JniType j_context, ::djinni_generated::NativeRenderPassConfig::JniType j_renderPass, jlong j_vpMatrix, jlong j_mMatrix, ::djinni_generated::NativeVec3D::JniType j_origin, jdouble j_screenPixelAsRealMeterFactor)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::MaskingObjectInterface>(nativeRef);
        ref->renderAsMask(::djinni_generated::NativeRenderingContextInterface::toCpp(jniEnv, j_context),
                          ::djinni_generated::NativeRenderPassConfig::toCpp(jniEnv, j_renderPass),
                          ::djinni::I64::toCpp(jniEnv, j_vpMatrix),
                          ::djinni::I64::toCpp(jniEnv, j_mMatrix),
                          ::djinni_generated::NativeVec3D::toCpp(jniEnv, j_origin),
                          ::djinni::F64::toCpp(jniEnv, j_screenPixelAsRealMeterFactor));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

} // namespace djinni_generated
