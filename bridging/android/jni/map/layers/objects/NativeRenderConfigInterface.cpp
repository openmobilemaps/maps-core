// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from layer_object.djinni

#include "NativeRenderConfigInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeGraphicsObjectInterface.h"

namespace djinni_generated {

NativeRenderConfigInterface::NativeRenderConfigInterface() : ::djinni::JniInterface<::RenderConfigInterface, NativeRenderConfigInterface>("io/openmobilemaps/mapscore/shared/map/layers/objects/RenderConfigInterface$CppProxy") {}

NativeRenderConfigInterface::~NativeRenderConfigInterface() = default;


CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_objects_RenderConfigInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::RenderConfigInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT ::djinni_generated::NativeGraphicsObjectInterface::JniType JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_objects_RenderConfigInterface_00024CppProxy_native_1getGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderConfigInterface>(nativeRef);
        auto r = ref->getGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jint JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_objects_RenderConfigInterface_00024CppProxy_native_1getRenderIndex(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::RenderConfigInterface>(nativeRef);
        auto r = ref->getRenderIndex();
        return ::djinni::release(::djinni::I32::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
