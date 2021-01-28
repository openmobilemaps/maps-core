#pragma once

#include "CameraInterface.h"
#include <vector>

class ExampleCamera: public CameraInterface {
public:
    ExampleCamera();
    
    virtual int64_t getMvpMatrix();

    virtual void addListener(const std::shared_ptr<CameraListenerInterface> & listener);

    virtual void removeListener(const std::shared_ptr<CameraListenerInterface> & listener);
private:
    std::vector<float> mvpMatrix;
};
