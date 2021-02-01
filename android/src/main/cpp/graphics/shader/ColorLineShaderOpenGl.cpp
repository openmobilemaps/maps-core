//
// Created by Christoph Maurhofer on 26.02.2020.
//

#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "ColorLineShaderOpenGl.h"

std::string ColorLineShaderOpenGl::getRectProgramName() {
    return "UBMAP_LineColorRectShaderOpenGl";
}

std::string ColorLineShaderOpenGl::getPointProgramName() {
    return "UBMAP_LineColorPointShaderOpenGl";
}

void ColorLineShaderOpenGl::setupRectProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getRectProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getRectVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getRectFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    OpenGlHelper::checkGlError("glAttachShader Vertex  ColorLine Rect");
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    OpenGlHelper::checkGlError("glAttachShader Fragment ColorLine Rect");
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables
    OpenGlHelper::checkGlError("glLinkProgram ColorLine Rect");

    openGlContext->storeProgram(programName, program);
}

void ColorLineShaderOpenGl::preRenderRect(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext>openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getRectProgramName());

    int mColorHandle = glGetUniformLocation(program, "vColor");
    // Set color for drawing the triangle
    glUniform4fv(mColorHandle, 1, &lineColor[0]);

    int mMiterHandle = glGetUniformLocation(program, "miter");
    // Set color for drawing the triangle
    glUniform1f(mMiterHandle, miter * zoom);
}

void ColorLineShaderOpenGl::setupPointProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getPointProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getPointVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getPointFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    OpenGlHelper::checkGlError("glAttachShader Vertex  ColorLine Point");
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    OpenGlHelper::checkGlError("glAttachShader Fragment ColorLine Point");
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables
    OpenGlHelper::checkGlError("glLinkProgram ColorLine Point");

    openGlContext->storeProgram(programName, program);
}

void ColorLineShaderOpenGl::preRenderPoint(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getPointProgramName());

    int mColorHandle = glGetUniformLocation(program, "vColor");
    // Set color for drawing the point
    glUniform4fv(mColorHandle, 1, &lineColor[0]);

    int mPointSizeHandle = glGetUniformLocation(program, "vPointSize");
    // Set point size (twice the stroke width)
    glUniform1f(mPointSizeHandle, miter * 2);
}

void ColorLineShaderOpenGl::setColor(float red, float green, float blue, float alpha) {
    lineColor = std::vector<float>{red, green, blue, alpha};
}

void ColorLineShaderOpenGl::setMiter(float miter_) {
    miter = miter_;
}

void ColorLineShaderOpenGl::setZoomFactor(float zoom_) {
    zoom = zoom_;
}

std::string ColorLineShaderOpenGl::getRectVertexShader() {
    return UBRendererShaderCode(precision
                                        highp float;
                                        uniform
                                        mat4 uMVPMatrix;
                                        attribute
                                        vec4 vPosition;
                                        attribute
                                        vec4 vNormal;
                                        uniform float miter;

                                        void main() {
                                            gl_Position = uMVPMatrix * (vPosition + (vNormal * vec4(miter, miter, 0.0, 0.0)));
                                        });
}

std::string ColorLineShaderOpenGl::getRectFragmentShader() {
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

std::string ColorLineShaderOpenGl::getPointVertexShader() {
    return UBRendererShaderCode(precision
                                        highp float;
                                        uniform
                                        mat4 uMVPMatrix;
                                        attribute
                                        vec4 vPosition;
                                        uniform
                                        highp float vPointSize;

                                        void main() {
                                            gl_PointSize = vPointSize;
                                            gl_Position = uMVPMatrix * vPosition;
                                        });
}

std::string ColorLineShaderOpenGl::getPointFragmentShader() {
    return UBRendererShaderCode(precision
                                        highp float;
                                        uniform
                                        vec4 vColor;

                                        void main() {
                                            vec2 coord = gl_PointCoord.st - vec2(0.5);  //from [0,1] to [-0.5,0.5]
                                            if (length(coord) > 0.5) {                 //outside of circle radius?
                                                discard;
                                            }

                                            gl_FragColor = vColor;
                                            gl_FragColor.a = 1.0;
                                            gl_FragColor *= vColor.a;
                                        });
}

std::shared_ptr<LineShaderProgramInterface>
ColorLineShaderOpenGl::asLineShaderProgramInterface() {
    return shared_from_this();
}
