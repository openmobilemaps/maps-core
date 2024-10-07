
#include "JNISchedulerCallbackInterface.h"

#ifdef __linux__
#include "JNISchedulerCallback.h"

std::shared_ptr<ThreadPoolCallbacks> JNISchedulerCallbackInterface::create() {
    return std::make_shared<JNISchedulerCallback>();
}

#else
// this is not used for iOS
std::shared_ptr<ThreadPoolCallbacks> JNISchedulerCallbackInterface::create() {
    return nullptr;
}

#endif
