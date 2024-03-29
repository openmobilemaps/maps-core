// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#include "NativeMapReadyCallbackInterface.h"  // my header
#include "NativeLayerReadyState.h"

namespace djinni_generated {

NativeMapReadyCallbackInterface::NativeMapReadyCallbackInterface() : ::djinni::JniInterface<::MapReadyCallbackInterface, NativeMapReadyCallbackInterface>() {}

NativeMapReadyCallbackInterface::~NativeMapReadyCallbackInterface() = default;

NativeMapReadyCallbackInterface::JavaProxy::JavaProxy(JniType j) : Handle(::djinni::jniGetThreadEnv(), j) { }

NativeMapReadyCallbackInterface::JavaProxy::~JavaProxy() = default;

void NativeMapReadyCallbackInterface::JavaProxy::stateDidUpdate(::LayerReadyState c_state) {
    auto jniEnv = ::djinni::jniGetThreadEnv();
    ::djinni::JniLocalScope jscope(jniEnv, 10);
    const auto& data = ::djinni::JniClass<::djinni_generated::NativeMapReadyCallbackInterface>::get();
    jniEnv->CallVoidMethod(Handle::get().get(), data.method_stateDidUpdate,
                           ::djinni::get(::djinni_generated::NativeLayerReadyState::fromCpp(jniEnv, c_state)));
    ::djinni::jniExceptionCheck(jniEnv);
}

} // namespace djinni_generated
