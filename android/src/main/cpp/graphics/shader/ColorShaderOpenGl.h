//
// Created by Christoph Maurhofer on 26.02.2020.
//

#ifndef MAPSDK_COLORSHADEROPENGL_H
#define MAPSDK_COLORSHADEROPENGL_H


#include <vector>
#include "ColorShaderInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "ShaderProgramInterface.h"

class ColorShaderOpenGl
        : public BaseShaderProgramOpenGl,
          public ColorShaderInterface,
          public ShaderProgramInterface,
          public std::enable_shared_from_this<ShaderProgramInterface> {
public:
    virtual std::shared_ptr <ShaderProgramInterface> asShaderProgramInterface();

    virtual std::string getProgramName();

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void setColor(float red, float green, float blue, float alpha);

protected:
    virtual std::string getVertexShader();

    virtual std::string getFragmentShader();

private:
    std::vector<float> color;
};


#endif //MAPSDK_COLORSHADEROPENGL_H
