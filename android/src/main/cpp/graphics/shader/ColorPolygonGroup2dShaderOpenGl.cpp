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

std::shared_ptr<ShaderProgramInterface> ColorPolygonGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }

std::string ColorPolygonGroup2dShaderOpenGl::getProgramName() { return "UBMAP_ColorPolygonGroupShaderOpenGl"; }

void ColorPolygonGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();
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
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        int lineStylesHandle = glGetUniformLocation(program, "polygonStyles");
        glUniform1fv(lineStylesHandle, sizeStyleValuesArray, &polygonStyles[0]);
        int numStylesHandle = glGetUniformLocation(program, "numStyles");
        glUniform1i(numStylesHandle, numStyles);
    }
}

void ColorPolygonGroup2dShaderOpenGl::setStyles(const ::SharedBytes & styles) {
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        this->polygonStyles.resize(sizeStyleValuesArray);
        if (styles.elementCount > 0) {
            std::memcpy(this->polygonStyles.data(), (void *) styles.address,
                    styles.elementCount * styles.bytesPerElement);
        }
        this->numStyles = styles.elementCount;
    }
}

std::string ColorPolygonGroup2dShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(precision highp float;

            uniform mat4 uMVPMatrix;
            attribute vec2 vPosition;
            attribute float vStyleIndex;
            // polygonStyles: {vec4 color, float opacity} - stride = 5
            uniform float polygonStyles[5 * 32];
            uniform int numStyles;

                                varying vec4 color;

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
                                    gl_Position = uMVPMatrix * vec4(vPosition, 0.0, 1.0);
                                });
}

std::string ColorPolygonGroup2dShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision highp float;

                                varying vec4 color;

                                void main() {
                                    gl_FragColor = color;
                                    gl_FragColor.a = 1.0;
                                    gl_FragColor *= color.a;
                                });
}
