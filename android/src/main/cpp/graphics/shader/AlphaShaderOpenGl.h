//
// Created by Christoph Maurhofer on 29.01.2021.
//

#ifndef MAPSDK_ALPHASHADEROPENGL_H
#define MAPSDK_ALPHASHADEROPENGL_H

#include "AlphaShaderInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "RenderingContextInterface.h"
#include "ShaderProgramInterface.h"

class AlphaShaderOpenGl : public BaseShaderProgramOpenGl,
                          public ShaderProgramInterface,
                          public AlphaShaderInterface,
                          public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void updateAlpha(float value) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

  protected:
    virtual std::string getFragmentShader() override;

  private:
    float alpha = 1;
};

#endif // MAPSDK_ALPHASHADEROPENGL_H
