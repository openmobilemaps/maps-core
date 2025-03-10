// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#include "NativeIcosahedronInterface.h"  // my header
#include "NativeGraphicsObjectInterface.h"
#include "NativeSharedBytes.h"
#include "NativeVec3D.h"

namespace djinni_generated {

NativeIcosahedronInterface::NativeIcosahedronInterface() : ::djinni::JniInterface<::IcosahedronInterface, NativeIcosahedronInterface>("io/openmobilemaps/mapscore/shared/graphics/objects/IcosahedronInterface$CppProxy") {}

NativeIcosahedronInterface::~NativeIcosahedronInterface() = default;

NativeIcosahedronInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeIcosahedronInterface::JavaProxy::~JavaProxy() = default;

void NativeIcosahedronInterface::JavaProxy::setVertices(const ::SharedBytes & c_vertices, const ::SharedBytes & c_indices, const ::Vec3D & c_origin) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeIcosahedronInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_setVertices,
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_vertices)),
                           ::djinni::get(::djinni_generated::NativeSharedBytes::fromCpp(jniEnv, c_indices)),
                           ::djinni::get(::djinni_generated::NativeVec3D::fromCpp(jniEnv, c_origin)));
    ::djinni::jniExceptionCheck(jniEnv);
}
/*not-null*/ std::shared_ptr<::GraphicsObjectInterface> NativeIcosahedronInterface::JavaProxy::asGraphicsObject() {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeIcosahedronInterface>::get();
    auto jret = jniEnv->CallObjectMethod(Handle::get().get(), data.method_asGraphicsObject);
    ::djinni::jniExceptionCheck(jniEnv);
    return ::djinni_generated::NativeGraphicsObjectInterface::toCpp(jniEnv, jret);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_IcosahedronInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::IcosahedronInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_IcosahedronInterface_00024CppProxy_native_1setVertices(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, ::djinni_generated::NativeSharedBytes::JniType j_vertices, ::djinni_generated::NativeSharedBytes::JniType j_indices, ::djinni_generated::NativeVec3D::JniType j_origin)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::IcosahedronInterface>(nativeRef);
        ref->setVertices(::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_vertices),
                         ::djinni_generated::NativeSharedBytes::toCpp(jniEnv, j_indices),
                         ::djinni_generated::NativeVec3D::toCpp(jniEnv, j_origin));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_objects_IcosahedronInterface_00024CppProxy_native_1asGraphicsObject(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::IcosahedronInterface>(nativeRef);
        auto r = ref->asGraphicsObject();
        return ::djinni::release(::djinni_generated::NativeGraphicsObjectInterface::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated
