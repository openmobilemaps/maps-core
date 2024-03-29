// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#include "NativeCoordinateConverterInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeCoord.h"

namespace djinni_generated {

NativeCoordinateConverterInterface::NativeCoordinateConverterInterface() : ::djinni::JniInterface<::CoordinateConverterInterface, NativeCoordinateConverterInterface>("io/openmobilemaps/mapscore/shared/map/coordinates/CoordinateConverterInterface$CppProxy") {}

NativeCoordinateConverterInterface::~NativeCoordinateConverterInterface() = default;

NativeCoordinateConverterInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeCoordinateConverterInterface::JavaProxy::~JavaProxy() = default;

::Coord NativeCoordinateConverterInterface::JavaProxy::convert(const ::Coord & c_coordinate) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeCoordinateConverterInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_convert,
                                         ::djinni::get(::djinni_generated::NativeCoord::fromCpp(jniEnv, c_coordinate)));
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeCoord::toCpp(jniEnv, jret);
}
int32_t NativeCoordinateConverterInterface::JavaProxy::getFrom() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeCoordinateConverterInterface>::get();
    auto jret = jniEnv->CallIntMethod(Handle::get().get(), data.method_getFrom);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::I32::toCpp(jniEnv, jret);
}
int32_t NativeCoordinateConverterInterface::JavaProxy::getTo() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeCoordinateConverterInterface>::get();
    auto jret = jniEnv->CallIntMethod(Handle::get().get(), data.method_getTo);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni::I32::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_coordinates_CoordinateConverterInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::CoordinateConverterInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_map_coordinates_CoordinateConverterInterface_00024CppProxy_native_1convert(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_coordinate)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::CoordinateConverterInterface>(nativeRef);
        auto r = ref->convert(::djinni_generated::NativeCoord::toCpp(jniEnv, j_coordinate));
        return ::djinni::release(::djinni_generated::NativeCoord::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jint JNICALL Java_io_openmobilemaps_mapscore_shared_map_coordinates_CoordinateConverterInterface_00024CppProxy_native_1getFrom(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::CoordinateConverterInterface>(nativeRef);
        auto r = ref->getFrom();
        return ::djinni::release(::djinni::I32::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jint JNICALL Java_io_openmobilemaps_mapscore_shared_map_coordinates_CoordinateConverterInterface_00024CppProxy_native_1getTo(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::CoordinateConverterInterface>(nativeRef);
        auto r = ref->getTo();
        return ::djinni::release(::djinni::I32::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
