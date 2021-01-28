#pragma once
#include "CameraInterface.h"
#include <vector>

class ExampleCamera: public CameraInterface {
public:
    ExampleCamera();
    
    int64_t getMvpMatrix();

    void addListener(const std::shared_ptr<CameraListenerInterface> & listener);

    void removeListener(const std::shared_ptr<CameraListenerInterface> & listener);
private:
    std::vector<float> mvpMatrix;
};
