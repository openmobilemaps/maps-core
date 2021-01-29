//
// Created by Christoph Maurhofer on 29.01.2021.
//

#ifndef MAPSDK_ALPHASHADEROPENGL_H
#define MAPSDK_ALPHASHADEROPENGL_H

#include "BaseShaderProgramOpenGl.h"
#include "ShaderProgramInterface.h"
#include "AlphaShaderInterface.h"
#include "RenderingContextInterface.h"

class AlphaShaderOpenGl
        : public BaseShaderProgramOpenGl,
          public ShaderProgramInterface,
          public AlphaShaderInterface,
          public std::enable_shared_from_this<ShaderProgramInterface> {

public:
    virtual std::string getProgramName();

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void updateAlpha(float value);

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface();
    
protected:
    virtual std::string getFragmentShader();

private:
    float alpha;

};


#endif //MAPSDK_ALPHASHADEROPENGL_H
