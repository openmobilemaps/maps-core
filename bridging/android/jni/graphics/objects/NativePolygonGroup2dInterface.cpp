// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativePolygonGroup2dInterface.h"  // my header
#include "NativeGraphicsObjectInterface.h"
#include "NativeSharedBytes.h"
#include "NativeVec3D.h"

namespace djinni_generated {

NativePolygonGroup2dInterface::NativePolygonGroup2dInterface() : ::djinni::JniInterface<::PolygonGroup2dInterface, NativePolygonGroup2dInterface>("io/openmobilemaps/mapscore/shared/graphics/objects/PolygonGroup2dInterface$CppProxy") {}

NativePolygonGroup2dInterface::~NativePolygonGroup2dInterface() = default;

NativePolygonGroup2dInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativePolygonGroup2dInterface::JavaProxy::~JavaProxy() = default;

void NativePolygonGroup2dInterface::JavaProxy::setVertices(const ::SharedBytes & c_vertices, const ::SharedBytes & c_indices, const ::Vec3D & c_origin) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativePolygonGroup2dInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_setVertices,
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_vertices)),
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_indices)),
                           ::djinni::get(::djinni_generated::NativeVec3D::fromCpp(jniEnv, c_origin)));
    ::djinni::jniExceptionCheck(jniEnv);
}
/*not-null*/ std::shared_ptr<::GraphicsObjectInterface> NativePolygonGroup2dInterface::JavaProxy::asGraphicsObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativePolygonGroup2dInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_asGraphicsObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeGraphicsObjectInterface::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_PolygonGroup2dInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::PolygonGroup2dInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_PolygonGroup2dInterface_00024CppProxy_native_1setVertices(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeSharedBytes::JniType j_vertices, ::djinni_generated::NativeSharedBytes::JniType j_indices, ::djinni_generated::NativeVec3D::JniType j_origin)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::PolygonGroup2dInterface>(nativeRef);
        ref->setVertices(::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_vertices),
                         ::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_indices),
                         ::djinni_generated::NativeVec3D::toCpp(jniEnv, j_origin));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_PolygonGroup2dInterface_00024CppProxy_native_1asGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::PolygonGroup2dInterface>(nativeRef);
        auto r = ref->asGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
