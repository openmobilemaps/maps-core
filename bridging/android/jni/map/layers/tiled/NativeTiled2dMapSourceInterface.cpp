// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#include "NativeTiled2dMapSourceInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeErrorManager.h"
#include "NativeLayerReadyState.h"
#include "NativeRectCoord.h"

namespace djinni_generated {

NativeTiled2dMapSourceInterface::NativeTiled2dMapSourceInterface() : ::djinni::JniInterface<::Tiled2dMapSourceInterface, NativeTiled2dMapSourceInterface>("io/openmobilemaps/mapscore/shared/map/layers/tiled/Tiled2dMapSourceInterface$CppProxy") {}

NativeTiled2dMapSourceInterface::~NativeTiled2dMapSourceInterface() = default;


CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::Tiled2dMapSourceInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1onVisibleBoundsChanged(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeRectCoord::JniType j_visibleBounds, jint j_curT, jdouble j_zoom)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->onVisibleBoundsChanged(::djinni_generated::NativeRectCoord::toCpp(jniEnv, j_visibleBounds),
                                    ::djinni::I32::toCpp(jniEnv, j_curT),
                                    ::djinni::F64::toCpp(jniEnv, j_zoom));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1setMinZoomLevelIdentifier(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_value)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->setMinZoomLevelIdentifier(::djinni::Optional<std::optional, ::djinni::I32>::toCpp(jniEnv, j_value));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1getMinZoomLevelIdentifier(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        auto r = ref->getMinZoomLevelIdentifier();
        return ::djinni::release(::djinni::Optional<std::optional, ::djinni::I32>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1setMaxZoomLevelIdentifier(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_value)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->setMaxZoomLevelIdentifier(::djinni::Optional<std::optional, ::djinni::I32>::toCpp(jniEnv, j_value));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1getMaxZoomLevelIdentifier(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        auto r = ref->getMaxZoomLevelIdentifier();
        return ::djinni::release(::djinni::Optional<std::optional, ::djinni::I32>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1pause(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->pause();
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1resume(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->resume();
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT ::djinni_generated::NativeLayerReadyState::JniType JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1isReadyToRenderOffscreen(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        auto r = ref->isReadyToRenderOffscreen();
        return ::djinni::release(::djinni_generated::NativeLayerReadyState::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1setErrorManager(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeErrorManager::JniType j_errorManager)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->setErrorManager(::djinni_generated::NativeErrorManager::toCpp(jniEnv, j_errorManager));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1forceReload(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->forceReload();
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_Tiled2dMapSourceInterface_00024CppProxy_native_1notifyTilesUpdates(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::Tiled2dMapSourceInterface>(nativeRef);
        ref->notifyTilesUpdates();
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

} // namespace djinni_generated
