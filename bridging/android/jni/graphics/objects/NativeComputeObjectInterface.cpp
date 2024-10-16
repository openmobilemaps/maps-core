// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativeComputeObjectInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeRenderingContextInterface.h"

namespace djinni_generated {

NativeComputeObjectInterface::NativeComputeObjectInterface() : ::djinni::JniInterface<::ComputeObjectInterface, NativeComputeObjectInterface>("io/openmobilemaps/mapscore/shared/graphics/objects/ComputeObjectInterface$CppProxy") {}

NativeComputeObjectInterface::~NativeComputeObjectInterface() = default;

NativeComputeObjectInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeComputeObjectInterface::JavaProxy::~JavaProxy() = default;

void NativeComputeObjectInterface::JavaProxy::compute(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & c_context, double c_screenPixelAsRealMeterFactor) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeComputeObjectInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_compute,
                           ::djinni::get(::djinni_generated::NativeRenderingContextInterface::fromCpp(jniEnv, c_context)),
                           ::djinni::get(::djinni::F64::fromCpp(jniEnv, c_screenPixelAsRealMeterFactor)));
    ::djinni::jniExceptionCheck(jniEnv);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_ComputeObjectInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::ComputeObjectInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_ComputeObjectInterface_00024CppProxy_native_1compute(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeRenderingContextInterface::JniType j_context, jdouble j_screenPixelAsRealMeterFactor)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::ComputeObjectInterface>(nativeRef);
        ref->compute(::djinni_generated::NativeRenderingContextInterface::toCpp(jniEnv, j_context),
                     ::djinni::F64::toCpp(jniEnv, j_screenPixelAsRealMeterFactor));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

} // namespace djinni_generated
