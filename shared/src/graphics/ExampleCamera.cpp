#include "ExampleCamera.h"

ExampleCamera::ExampleCamera() : mvpMatrix(16,0) {
}

int64_t ExampleCamera::getMvpMatrix() {
    return (int64_t)mvpMatrix.data();
}

void ExampleCamera::addListener(const std::shared_ptr<CameraListenerInterface> & listener) {}

void ExampleCamera::removeListener(const std::shared_ptr<CameraListenerInterface> & listener) {}
