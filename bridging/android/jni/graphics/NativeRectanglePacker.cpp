// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from packer.djinni

#include "NativeRectanglePacker.h"  // my header
#include "Marshal.hpp"
#include "NativeRectanglePackerPage.h"
#include "NativeVec2I.h"

namespace djinni_generated {

NativeRectanglePacker::NativeRectanglePacker() : ::djinni::JniInterface<::RectanglePacker, NativeRectanglePacker>("io/openmobilemaps/mapscore/shared/graphics/RectanglePacker$CppProxy") {}

NativeRectanglePacker::~NativeRectanglePacker() = default;


CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RectanglePacker_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::RectanglePacker>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_io_openmobilemaps_mapscore_shared_graphics_RectanglePacker_pack(JNIEnv* jniEnv, jobject /*this*/, jobject j_rectangles, ::djinni_generated::NativeVec2I::JniType j_maxPageSize)
{
    try {
        auto r = ::RectanglePacker::pack(::djinni::Map<::djinni::String, ::djinni_generated::NativeVec2I>::toCpp(jniEnv, j_rectangles),
                                         ::djinni_generated::NativeVec2I::toCpp(jniEnv, j_maxPageSize));
        return ::djinni::release(::djinni::List<::djinni_generated::NativeRectanglePackerPage>::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

} // namespace djinni_generated