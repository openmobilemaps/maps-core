// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#include "NativeRasterShaderInterface.h"  // my header
#include "NativeRasterShaderStyle.h"
#include "NativeShaderProgramInterface.h"

namespace djinni_generated {

NativeRasterShaderInterface::NativeRasterShaderInterface() : ::djinni::JniInterface<::RasterShaderInterface, NativeRasterShaderInterface>("io/openmobilemaps/mapscore/shared/graphics/shader/RasterShaderInterface$CppProxy") {}

NativeRasterShaderInterface::~NativeRasterShaderInterface() = default;


CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_shader_RasterShaderInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::RasterShaderInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_shader_RasterShaderInterface_00024CppProxy_native_1setStyle(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_style)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RasterShaderInterface>(nativeRef);
        ref->setStyle(::djinni_generated::NativeRasterShaderStyle::toCpp(jniEnv, j_style));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_shader_RasterShaderInterface_00024CppProxy_native_1asShaderProgramInterface(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RasterShaderInterface>(nativeRef);
        auto r = ref->asShaderProgramInterface();
        return ::djinni::release(::djinni_generated::NativeShaderProgramInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
