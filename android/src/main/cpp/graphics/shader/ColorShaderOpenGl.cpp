//
// Created by Christoph Maurhofer on 26.02.2020.
//

#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "ColorShaderOpenGl.h"

std::string ColorShaderOpenGl::getProgramName() {
    return "UBMAP_ColorShaderOpenGl";
}

void ColorShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    OpenGlHelper::checkGlError("glAttachShader Vertex  Color");
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    OpenGlHelper::checkGlError("glAttachShader Fragment Color");
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables
    OpenGlHelper::checkGlError("glLinkProgram Color");

    openGlContext->storeProgram(programName, program);
}

void ColorShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    int mColorHandle = glGetUniformLocation(program, "vColor");
    // Set color for drawing the triangle
    glUniform4fv(mColorHandle, 1, &color[0]);
}

void ColorShaderOpenGl::setColor(float red, float green, float blue, float alpha) {
    color = std::vector<float>{red, green, blue, alpha};
}

std::string ColorShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(precision
                                        highp float;
                                        uniform
                                        mat4 uMVPMatrix;
                                        attribute
                                        vec4 vPosition;

                                        void main() {
                                            gl_Position = uMVPMatrix * vPosition;
                                        });
}

std::string ColorShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision
                                        mediump float;
                                        uniform
                                        vec4 vColor;

                                        void main() {
                                            gl_FragColor = vColor;
                                            gl_FragColor.a = 1.0;
                                            gl_FragColor *= vColor.a;
                                        });
}

std::shared_ptr<ShaderProgramInterface>
ColorShaderOpenGl::asShaderProgramInterface() {
    return shared_from_this();
}
