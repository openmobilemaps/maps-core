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
    virtual std::shared_ptr <ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setColor(float red, float green, float blue, float alpha) override;

protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

private:
    std::vector<float> color;
};


#endif //MAPSDK_COLORSHADEROPENGL_H
