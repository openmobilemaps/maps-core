// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#include "NativeDefaultTiled2dMapLayerConfigs.h"  // my header
#include "Marshal.hpp"
#include "NativeTiled2dMapLayerConfig.h"

namespace djinni_generated {

NativeDefaultTiled2dMapLayerConfigs::NativeDefaultTiled2dMapLayerConfigs() : ::djinni::JniInterface<::DefaultTiled2dMapLayerConfigs, NativeDefaultTiled2dMapLayerConfigs>("io/openmobilemaps/mapscore/shared/map/layers/tiled/DefaultTiled2dMapLayerConfigs$CppProxy") {}

NativeDefaultTiled2dMapLayerConfigs::~NativeDefaultTiled2dMapLayerConfigs() = default;


CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_DefaultTiled2dMapLayerConfigs_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::DefaultTiled2dMapLayerConfigs>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_map_layers_tiled_DefaultTiled2dMapLayerConfigs_webMercator(JNIEnv* jniEnv, jobject /*this*/, jstring j_layerName, jstring j_urlFormat)
{
    try {
        auto r = ::DefaultTiled2dMapLayerConfigs::webMercator(::djinni::String::toCpp(jniEnv, j_layerName),
                                                              ::djinni::String::toCpp(jniEnv, j_urlFormat));
        return ::djinni::release(::djinni_generated::NativeTiled2dMapLayerConfig::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
