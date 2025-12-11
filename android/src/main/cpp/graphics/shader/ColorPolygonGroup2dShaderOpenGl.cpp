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
#include <cstring>

ColorPolygonGroup2dShaderOpenGl::ColorPolygonGroup2dShaderOpenGl(bool isStriped, bool projectOntoUnitSphere)
        : isStriped(isStriped),
          programName(std::string("UBMAP_ColorPolygonGroupShaderOpenGl_") + (projectOntoUnitSphere ? "UnitSphere_" : "")
          + (isStriped ? "striped" : "std")) {
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

    GLuint blockIdx = glGetUniformBlockIndex(program, "PolygonStyleCollection");
    if (blockIdx == GL_INVALID_INDEX) {
        LogError <<= "Uniform block PolygonStyleCollection not found";
    }
    glUniformBlockBinding(program, blockIdx, STYLE_UBO_BINDING);
}

void ColorPolygonGroup2dShaderOpenGl::setupGlObjects(const std::shared_ptr<::OpenGlContext> &context) {
    if (polygonStyleBuffer == 0) {
        glGenBuffers(1, &polygonStyleBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, polygonStyleBuffer);
        // maximum number of polygonStyles and numStyles, padded to 16 byte alignment
        glBufferData(GL_UNIFORM_BUFFER, sizeStyleValuesArray * sizeof(GLfloat) + sizeof(GLint) + 3 * sizeof(GLint), nullptr,
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void ColorPolygonGroup2dShaderOpenGl::clearGlObjects() {
    if (polygonStyleBuffer > 0) {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        polygonStyleBuffer = 0;
        stylesUpdated = true;
        glDeleteBuffers(1, &polygonStyleBuffer);
    }
}

void ColorPolygonGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) {
    BaseShaderProgramOpenGl::preRender(context, isScreenSpaceCoords);

    glBindBufferBase(GL_UNIFORM_BUFFER, STYLE_UBO_BINDING, polygonStyleBuffer);

    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        if (stylesUpdated) {
            stylesUpdated = false;
            glBindBuffer(GL_UNIFORM_BUFFER, polygonStyleBuffer);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, numStyles * sizeStyleValues * sizeof(GLfloat), &polygonStyles[0]);
            glBufferSubData(GL_UNIFORM_BUFFER, MAX_NUM_STYLES * sizeStyleValues * sizeof(GLfloat), sizeof(GLint), &numStyles);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
    }
}

bool ColorPolygonGroup2dShaderOpenGl::isRenderable() {
    std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
    return numStyles > 0;
}

void ColorPolygonGroup2dShaderOpenGl::setStyles(const ::SharedBytes & styles) {
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        if (styles.elementCount > 0) {
            std::memcpy(this->polygonStyles.data(), (void *) styles.address,
                    styles.elementCount * styles.bytesPerElement);
        }

        this->numStyles = styles.elementCount;
        stylesUpdated = true;
    }
}

std::string ColorPolygonGroup2dShaderOpenGl::getPolygonStylesUBODefinition(bool isStriped) {
    if (isStriped) {
        return OMMShaderCode(
                struct StripedPolygonStyle {
                    float colorR; // 0
                    float colorG; // 1
                    float colorB; // 2
                    float colorA; // 3
                    float opacity; // 4
                    float stripeWidth; // 5
                    float gapWidth; // 6
                }; // padded to 8 floats

                layout (std140) uniform PolygonStyleCollection {
                    StripedPolygonStyle polygonStyles[) + std::to_string(MAX_NUM_STYLES) + OMMShaderCode(];
                    lowp int numStyles;
                    // padding
                } uPolygonStyles;
        );
    } else {
        return OMMShaderCode(
                struct PolygonStyle {
                    float colorR; // 0
                    float colorG; // 1
                    float colorB; // 2
                    float colorA; // 3
                    float opacity; // 4
                }; // padded to 8 floats

                layout (std140) uniform PolygonStyleCollection {
                    PolygonStyle polygonStyles[) + std::to_string(MAX_NUM_STYLES) + OMMShaderCode(];
                    lowp int numStyles;
                    // padding
                } uPolygonStyles;
        );
    }
}

std::string ColorPolygonGroup2dShaderOpenGl::getVertexShader() {
    // TO_CHANGE
    return isStriped ? OMMVersionedGlesShaderCodeWithFrameUBO(300 es,
                // Striped Shader
                precision highp float;

                ) + getPolygonStylesUBODefinition(isStriped) + OMMShaderCode(

                uniform vec4 uOriginOffset;

                in vec3 vPosition;
                in float vStyleIndex;

                flat out int styleIndex;
                out vec2 uv;

                void main() {
                    gl_Position = uFrameUniforms.vpMatrix * (vec4(vPosition, 1.0) + uOriginOffset);

                    styleIndex = clamp(int(floor(vStyleIndex + 0.5)), 0, uPolygonStyles.numStyles);
                    uv = vPosition.xy;
                }
            ) : OMMVersionedGlesShaderCodeWithFrameUBO(300 es,
                // Default Color Shader
                precision highp float;

                ) + getPolygonStylesUBODefinition(isStriped) + OMMShaderCode(

                uniform vec4 uOriginOffset;

                in vec3 vPosition;
                in float vStyleIndex;

                flat out int styleIndex;

                void main() {
                    gl_Position = uFrameUniforms.vpMatrix * (vec4(vPosition, 1.0) + uOriginOffset);

                    styleIndex = clamp(int(floor(vStyleIndex + 0.5)), 0, uPolygonStyles.numStyles);
                });
}

std::string ColorPolygonGroup2dShaderOpenGl::getFragmentShader() {
    return isStriped ? OMMVersionedGlesShaderCode(300 es,
                        // Striped Shader
                        precision highp float;

                        ) + getPolygonStylesUBODefinition(isStriped) + OMMShaderCode(

                        uniform vec2 scaleFactors;

                        flat in int styleIndex;
                        in vec2 uv;

                        out vec4 fragmentColor;

                        void main() {
                            float disPx = (uv.x + uv.y) / scaleFactors.y;
                            float totalPx = uPolygonStyles.polygonStyles[styleIndex].stripeWidth + uPolygonStyles.polygonStyles[styleIndex].gapWidth;
                            float adjLineWPx = uPolygonStyles.polygonStyles[styleIndex].stripeWidth / scaleFactors.y * scaleFactors.x;
                            if (mod(disPx, totalPx) > adjLineWPx) {
                                fragmentColor = vec4(0.0);
                                return;
                            }

                            vec4 color = vec4(uPolygonStyles.polygonStyles[styleIndex].colorR, uPolygonStyles.polygonStyles[styleIndex].colorG,
                                         uPolygonStyles.polygonStyles[styleIndex].colorB, uPolygonStyles.polygonStyles[styleIndex].colorA * uPolygonStyles.polygonStyles[styleIndex].opacity);
                            fragmentColor = color;
                            fragmentColor.a = 1.0;
                            fragmentColor *= color.a;
                        }
                  ) : OMMVersionedGlesShaderCode(300 es,
                        // Default Color Shader
                        precision highp float;

                        ) + getPolygonStylesUBODefinition(isStriped) + OMMShaderCode(

                        flat in int styleIndex;
                        out vec4 fragmentColor;

                        void main() {
                            vec4 color = vec4(uPolygonStyles.polygonStyles[styleIndex].colorR, uPolygonStyles.polygonStyles[styleIndex].colorG,
                                         uPolygonStyles.polygonStyles[styleIndex].colorB, uPolygonStyles.polygonStyles[styleIndex].colorA * uPolygonStyles.polygonStyles[styleIndex].opacity);
                            fragmentColor = color;
                            fragmentColor.a = 1.0;
                            fragmentColor *= color.a;
                        });
}
