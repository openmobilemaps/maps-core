// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from maps_core.djinni

#include "NativeMapsCoreSharedModule.h"  // my header
#include "Marshal.hpp"

namespace djinni_generated {

NativeMapsCoreSharedModule::NativeMapsCoreSharedModule() : ::djinni::JniInterface<::MapsCoreSharedModule, NativeMapsCoreSharedModule>("io/openmobilemaps/mapscore/shared/MapsCoreSharedModule$CppProxy") {}

NativeMapsCoreSharedModule::~NativeMapsCoreSharedModule() = default;


CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_MapsCoreSharedModule_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::MapsCoreSharedModule>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jstring JNICALL Java_io_openmobilemaps_mapscore_shared_MapsCoreSharedModule_version(JNIEnv* jniEnv, jobject /*this*/)
{
    try {
        auto r = ::MapsCoreSharedModule::version();
        return ::djinni::release(::djinni::String::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
