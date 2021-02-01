//
// Created by Christoph Maurhofer on 26.02.2020.
//

#ifndef MAPSDK_COLORLINESHADEROPENGL_H
#define MAPSDK_COLORLINESHADEROPENGL_H


#include <vector>
#include "LineShaderProgramInterface.h"
#include "ColorLineShaderInterface.h"
#include "BaseShaderProgramOpenGl.h"

class ColorLineShaderOpenGl
        : public BaseShaderProgramOpenGl,
          public LineShaderProgramInterface,
          public ColorLineShaderInterface,
          public std::enable_shared_from_this<LineShaderProgramInterface> {
public:

    virtual std::shared_ptr <LineShaderProgramInterface> asLineShaderProgramInterface();

    virtual std::string getRectProgramName();

    virtual void setupRectProgram(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void preRenderRect(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual std::string getPointProgramName();

    virtual void setupPointProgram(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void preRenderPoint(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual std::string getRectVertexShader();

    virtual std::string getRectFragmentShader();

    virtual std::string getPointVertexShader();

    virtual std::string getPointFragmentShader();

    virtual void setColor(float red, float green, float blue, float alpha);

    virtual void setMiter(float miter);

private:
    std::vector<float> lineColor;
    float miter;
};


#endif //MAPSDK_COLORLINESHADEROPENGL_H
