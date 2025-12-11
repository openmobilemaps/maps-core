#include "WebMapCallback.h"

#include <atomic>
#include <memory>

std::shared_ptr<WebMapCallbackInterface> WebMapCallbackInterface::create() {
    return std::make_shared<WebMapCallback>();
}

void WebMapCallback::invalidate() {
    invalid = true;
}

void WebMapCallback::onMapResumed() {
    invalid = true;
}

bool WebMapCallback::getAndResetInvalid() {
    return invalid.exchange(false);
}

std::shared_ptr<MapCallbackInterface> WebMapCallback::asCallbackInterface() {
    return std::static_pointer_cast<MapCallbackInterface>(shared_from_this());
}
