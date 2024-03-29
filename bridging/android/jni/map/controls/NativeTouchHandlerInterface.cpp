// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from touch_handler.djinni

#include "NativeTouchHandlerInterface.h"  // my header
#include "Marshal.hpp"
#include "NativeTouchEvent.h"
#include "NativeTouchInterface.h"

namespace djinni_generated {

NativeTouchHandlerInterface::NativeTouchHandlerInterface() : ::djinni::JniInterface<::TouchHandlerInterface, NativeTouchHandlerInterface>("io/openmobilemaps/mapscore/shared/map/controls/TouchHandlerInterface$CppProxy") {}

NativeTouchHandlerInterface::~NativeTouchHandlerInterface() = default;

NativeTouchHandlerInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeTouchHandlerInterface::JavaProxy::~JavaProxy() = default;

void NativeTouchHandlerInterface::JavaProxy::onTouchEvent(const ::TouchEvent & c_touchEvent) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTouchHandlerInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_onTouchEvent,
                           ::djinni::get(::djinni_generated::NativeTouchEvent::fromCpp(jniEnv, c_touchEvent)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeTouchHandlerInterface::JavaProxy::insertListener(const /*not-null*/ std::shared_ptr<::TouchInterface> & c_listener, int32_t c_index) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTouchHandlerInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_insertListener,
                           ::djinni::get(::djinni_generated::NativeTouchInterface::fromCpp(jniEnv, c_listener)),
                           ::djinni::get(::djinni::I32::fromCpp(jniEnv, c_index)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeTouchHandlerInterface::JavaProxy::addListener(const /*not-null*/ std::shared_ptr<::TouchInterface> & c_listener) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTouchHandlerInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_addListener,
                           ::djinni::get(::djinni_generated::NativeTouchInterface::fromCpp(jniEnv, c_listener)));
    ::djinni::jniExceptionCheck(jniEnv);
}
void NativeTouchHandlerInterface::JavaProxy::removeListener(const /*not-null*/ std::shared_ptr<::TouchInterface> & c_listener) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeTouchHandlerInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_removeListener,
                           ::djinni::get(::djinni_generated::NativeTouchInterface::fromCpp(jniEnv, c_listener)));
    ::djinni::jniExceptionCheck(jniEnv);
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_controls_TouchHandlerInterface_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        delete reinterpret_cast<::djinni::CppProxyHandle<::TouchHandlerInterface>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_controls_TouchHandlerInterface_00024CppProxy_native_1onTouchEvent(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_touchEvent)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TouchHandlerInterface>(nativeRef);
        ref->onTouchEvent(::djinni_generated::NativeTouchEvent::toCpp(jniEnv, j_touchEvent));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_controls_TouchHandlerInterface_00024CppProxy_native_1insertListener(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_listener, jint j_index)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TouchHandlerInterface>(nativeRef);
        ref->insertListener(::djinni_generated::NativeTouchInterface::toCpp(jniEnv, j_listener),
                            ::djinni::I32::toCpp(jniEnv, j_index));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_controls_TouchHandlerInterface_00024CppProxy_native_1addListener(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_listener)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TouchHandlerInterface>(nativeRef);
        ref->addListener(::djinni_generated::NativeTouchInterface::toCpp(jniEnv, j_listener));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_shared_map_controls_TouchHandlerInterface_00024CppProxy_native_1removeListener(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_listener)
{
    try {
        const auto& ref = ::djinni::objectFromHandleAddress<::TouchHandlerInterface>(nativeRef);
        ref->removeListener(::djinni_generated::NativeTouchInterface::toCpp(jniEnv, j_listener));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

} // namespace djinni_generated
