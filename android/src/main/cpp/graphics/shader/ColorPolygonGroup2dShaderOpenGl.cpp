/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ColorPolygonGroup2dShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "ColorPolygonGroup2dShaderOpenGl.h"

ColorPolygonGroup2dShaderOpenGl::ColorPolygonGroup2dShaderOpenGl(bool isStriped)
        : isStriped(isStriped),
          programName(std::string("UBMAP_ColorPolygonGroupShaderOpenGl_") + (isStriped ? "striped" : "std")) {
    this->polygonStyles.resize(sizeStyleValuesArray);
}

std::shared_ptr<ShaderProgramInterface> ColorPolygonGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }

std::string ColorPolygonGroup2dShaderOpenGl::getProgramName() { return programName; }

void ColorPolygonGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program

    glAttachShader(program, vertexShader); // add the vertex shader to program

    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables

    openGlContext->storeProgram(programName, program);
}

void ColorPolygonGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);

    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        if (numStyles == 0) {
            return;
        }
        int lineStylesHandle = glGetUniformLocation(program, "polygonStyles");
        glUniform1fv(lineStylesHandle, numStyles * sizeStyleValues, &polygonStyles[0]);
        int numStylesHandle = glGetUniformLocation(program, "numStyles");
        glUniform1i(numStylesHandle, numStyles);
    }
}

void ColorPolygonGroup2dShaderOpenGl::setStyles(const ::SharedBytes & styles) {
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        if (styles.elementCount > 0) {
            std::memcpy(this->polygonStyles.data(), (void *) styles.address,
                    styles.elementCount * styles.bytesPerElement);
        }

        this->numStyles = styles.elementCount;
    }
}

std::string ColorPolygonGroup2dShaderOpenGl::getVertexShader() {
    return isStriped ? OMMVersionedGlesShaderCode(320 es,
                // Striped Shader
                precision highp float;

                uniform mat4 uvpMatrix;
                in vec2 vPosition;
                in float vStyleIndex;
                // polygonStyles: {vec4 color, float opacity, stripe width, gap width} - stride = 7
                uniform float polygonStyles[7 * 16];
                uniform int numStyles;

                out vec4 color;
                out vec2 stripeInfo;
                out vec2 uv;

                void main() {
                  int styleIndex = int(floor(vStyleIndex + 0.5));
                  if (styleIndex < 0) {
                      styleIndex = 0;
                  } else if (styleIndex > numStyles) {
                      styleIndex = numStyles;
                  }
                  styleIndex = styleIndex * 5;
                  color = vec4(polygonStyles[styleIndex], polygonStyles[styleIndex + 1],
                               polygonStyles[styleIndex + 2], polygonStyles[styleIndex + 3] * polygonStyles[styleIndex + 4]);
                  stripeInfo = vec2(polygonStyles[styleIndex + 5], polygonStyles[styleIndex + 6]);
                  uv = vPosition;
                  gl_Position = uvpMatrix * vec4(vPosition, 0.0, 1.0);
                }
            ) : OMMVersionedGlesShaderCode(320 es,
                // Default Color Shader
                precision highp float;

                uniform mat4 uvpMatrix;
                in vec2 vPosition;
                in float vStyleIndex;
                // polygonStyles: {vec4 color, float opacity} - stride = 5
                uniform float polygonStyles[5 * 16];
                uniform int numStyles;

                out vec4 color;

                void main() {
                    int styleIndex = int(floor(vStyleIndex + 0.5));
                    if (styleIndex < 0) {
                        styleIndex = 0;
                    } else if (styleIndex > numStyles) {
                        styleIndex = numStyles;
                    }
                    styleIndex = styleIndex * 5;
                    color = vec4(polygonStyles[styleIndex], polygonStyles[styleIndex + 1],
                                 polygonStyles[styleIndex + 2], polygonStyles[styleIndex + 3] * polygonStyles[styleIndex + 4]);
                    gl_Position = uvpMatrix * vec4(vPosition, 0.0, 1.0);
                });
}

std::string ColorPolygonGroup2dShaderOpenGl::getFragmentShader() {
    return isStriped ? OMMVersionedGlesShaderCode(320 es,
                        // Striped Shader
                        precision highp float;

                        uniform vec2 scaleFactors;

                        in vec4 color;
                        in vec2 stripeInfo;
                        in vec2 uv;

                        out vec4 fragmentColor;

                        void main() {
                            float disPx = (uv.x + uv.y) / scaleFactors.y;
                            float totalPx = stripeInfo.x + stripeInfo.y;
                            float adjLineWPx = stripeInfo.x / scaleFactors.y * scaleFactors.x;
                            if (mod(disPx, totalPx) > adjLineWPx) {
                                fragmentColor = vec4(0.0);
                                return;
                            }

                            fragmentColor = color;
                            fragmentColor.a = 1.0;
                            fragmentColor *= color.a;
                        }
                  ) : OMMVersionedGlesShaderCode(320 es,
                        // Default Color Shader
                        precision highp float;

                        in vec4 color;
                        out vec4 fragmentColor;

                        void main() {
                          fragmentColor = color;
                          fragmentColor.a = 1.0;
                          fragmentColor *= color.a;
                        });
}
