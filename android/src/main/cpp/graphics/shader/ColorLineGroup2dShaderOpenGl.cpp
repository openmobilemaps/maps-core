/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ColorLineGroup2dShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include <cassert>
#include <cstring>

ColorLineGroup2dShaderOpenGl::ColorLineGroup2dShaderOpenGl(bool projectOntoUnitSphere, bool simple)
        : projectOntoUnitSphere(projectOntoUnitSphere),
          isSimpleLine(simple),
          sizeLineValues(simple ? sizeSimpleLineValues : sizeFullLineValues),
          sizeLineValuesArray((simple ? sizeSimpleLineValues : sizeFullLineValues) * MAX_NUM_STYLES),
          programName(simple ? (projectOntoUnitSphere
                                ? "UBMAP_ColorSimpleLineGroupUnitSphereShaderOpenGl"
                                : "UBMAP_ColorSimpleLineGroupShaderOpenGl") : projectOntoUnitSphere
                                                                              ? "UBMAP_ColorLineGroupUnitSphereShaderOpenGl"
                                                                              : "UBMAP_ColorLineGroupShaderOpenGl") {
    lineValues.resize(sizeLineValuesArray);
}

std::shared_ptr<ShaderProgramInterface> ColorLineGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }

std::string ColorLineGroup2dShaderOpenGl::getProgramName() { return programName; }

void ColorLineGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    // Bind LineStyleCollection at binding index 0
    GLuint lineStyleUniformBlockIdx = glGetUniformBlockIndex(program, "LineStyleCollection");
    if (lineStyleUniformBlockIdx == GL_INVALID_INDEX) {
        LogError <<= "Uniform block LineStyleCollection not found";
    }
    glUniformBlockBinding(program, lineStyleUniformBlockIdx, 0);
}

void ColorLineGroup2dShaderOpenGl::setupGlObjects(const std::shared_ptr<::OpenGlContext> &context) {
    if (lineStyleBuffer == 0) {
        glGenBuffers(1, &lineStyleBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, lineStyleBuffer);
        // maximum number of lineStyles and numStyles, padded to 16 byte alignment
        glBufferData(GL_UNIFORM_BUFFER, sizeLineValuesArray * sizeof(GLfloat) + sizeof(GLint) + 3 * sizeof(GLint), nullptr,
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    int program = context->getProgram(programName);
    dashingScaleFactorHandle = glGetUniformLocation(program, "dashingScaleFactor");
    timeFrameDeltaHandle = glGetUniformLocation(program, "timeFrameDeltaSeconds");
}

void ColorLineGroup2dShaderOpenGl::clearGlObjects() {
    if (lineStyleBuffer > 0) {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        lineStyleBuffer = 0;
        stylesUpdated = true;
        glDeleteBuffers(1, &lineStyleBuffer);
    }
    dashingScaleFactorHandle = -1;
    timeFrameDeltaHandle = -1;
}

void ColorLineGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, lineStyleBuffer); // LineStyleCollection is at binding index 0

    {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        if (numStyles == 0) {
            return;
        }

        if (dashingScaleFactorHandle >= 0) {
            glUniform1f(dashingScaleFactorHandle, dashingScaleFactor);
        }
        if (timeFrameDeltaHandle >= 0) {
            std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
            glUniform1f(timeFrameDeltaHandle, (float) openGlContext->getDeltaTimeMs() / 1000.0f);
        }

        if (stylesUpdated) {
            stylesUpdated = false;
            glBindBuffer(GL_UNIFORM_BUFFER, lineStyleBuffer);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, numStyles * sizeLineValues * sizeof(GLfloat), &lineValues[0]);
            glBufferSubData(GL_UNIFORM_BUFFER, MAX_NUM_STYLES * sizeLineValues * sizeof(GLfloat), sizeof(GLint), &numStyles);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
    }
}

bool ColorLineGroup2dShaderOpenGl::isRenderable() {
    std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
    return numStyles > 0;
}

void ColorLineGroup2dShaderOpenGl::setStyles(const ::SharedBytes & styles) {
    assert(styles.elementCount <= MAX_NUM_STYLES);
    assert(styles.elementCount * styles.bytesPerElement <= lineValues.size() * sizeof(float));
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        if(styles.elementCount > 0) {
            std::memcpy(lineValues.data(), (void *)styles.address, styles.elementCount * styles.bytesPerElement);
        }

        numStyles = styles.elementCount;
        stylesUpdated = true;
    }
}

void ColorLineGroup2dShaderOpenGl::setDashingScaleFactor(float factor) {
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        this->dashingScaleFactor = factor;
    }
}

std::string ColorLineGroup2dShaderOpenGl::getLineStylesUBODefinition(bool isSimpleLine) {
    if (isSimpleLine) {
        return OMMShaderCode(
                struct SimpleLineStyle {
                    float width; // 0
                    float colorR; // 1
                    float colorG; // 2
                    float colorB; // 3
                    float colorA; // 4
                    float widthAsPixels; // 5
                    float opacity; // 6
                    float capType; // 7
                };

                layout (std140) uniform LineStyleCollection {
                    SimpleLineStyle lineValues[) + std::to_string(MAX_NUM_STYLES) + OMMShaderCode(];
                    lowp int numStyles;
                } uLineStyles;
        );
    } else {
        return OMMShaderCode(
                struct LineStyle {
                    float width; // 0
                    float colorR; // 1
                    float colorG; // 2
                    float colorB; // 3
                    float colorA; // 4
                    float gapColorR; // 5
                    float gapColorG; // 6
                    float gapColorB; // 7
                    float gapColorA; // 8
                    float widthAsPixels; // 9
                    float opacity; // 10
                    float blur; // 11
                    float capType; // 12
                    float numDashValues; // 13
                    float dashArray0; // 14
                    float dashArray1; // 15
                    float dashArray2; // 16
                    float dashArray3; // 17
                    float dashFade; // 18
                    float dashAnimationSpeed; // 19
                    float offset; // 20
                    float dotted; // 21
                    float dottedSkew; // 22
                };

                layout (std140) uniform LineStyleCollection {
                    LineStyle lineValues[) + std::to_string(MAX_NUM_STYLES) + OMMShaderCode(];
                    lowp int numStyles;
                } uLineStyles;
        );
    }
}

std::string ColorLineGroup2dShaderOpenGl::getVertexShader() {
    bool is3d = projectOntoUnitSphere;
    bool isSimple = isSimpleLine;

    return
        OMMVersionedGlesShaderCode(320 es,
        precision highp float;
        uniform mat4 uvpMatrix;
        uniform vec4 originOffset;
        uniform float scalingFactor;
        ) +

        (is3d ? OMMShaderCode(
            in vec3 position;
            in vec3 extrude;
        ) :
        OMMShaderCode(
            in vec2 position;
            in vec2 extrude;
        )) +

        OMMShaderCode(
            in float lineSide;
            in float lengthPrefix;
            in float lengthCorrection;
            in float stylingIndex;

            out vec4 outColor;
            flat out int outStylingIndex;
        )

        + (isSimple ? " " :
           OMMShaderCode(
                out float outLengthPrefix;
                out float outLineSide;
        ))

        + getLineStylesUBODefinition(isSimple) +

        OMMShaderCode(
            void main() {
                float fStylingIndex = mod(stylingIndex, 256.0);
                int index = clamp(int(floor(fStylingIndex + 0.5)), 0, uLineStyles.numStyles);
                float width = uLineStyles.lineValues[index].width / 2.0 * scalingFactor;
                ) +

                (is3d ? OMMShaderCode(
                    vec4 extendedPosition = vec4(position + extrude * width, 1.0) + originOffset;
                ) :
                OMMShaderCode(
                    vec4 extendedPosition = vec4(position + extrude * width, 0.0, 1.0) + originOffset;
                )) +

                (isSimple ? " " :
                OMMShaderCode(
                    outLengthPrefix = lengthPrefix + lengthCorrection * width;
                    outLineSide = lineSide;
                )) +

                OMMShaderCode(
                    outStylingIndex = index;
                    outColor = vec4(uLineStyles.lineValues[index].colorR,
                                    uLineStyles.lineValues[index].colorG,
                                    uLineStyles.lineValues[index].colorB,
                                    uLineStyles.lineValues[index].colorA);

                    gl_Position = uvpMatrix * extendedPosition;
                }
        );
}

std::string ColorLineGroup2dShaderOpenGl::getSimpleLineFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
           precision highp float;
           )

           + getLineStylesUBODefinition(isSimpleLine) +

           OMMShaderCode(
           in vec4 outColor;
           flat in int outStylingIndex;
           out vec4 fragmentColor;

           void main() {
               float opacity = uLineStyles.lineValues[outStylingIndex].opacity;

               fragmentColor = outColor;
               fragmentColor.a = 1.0;
               fragmentColor *= outColor.a * opacity;
           });
}

std::string ColorLineGroup2dShaderOpenGl::getLineFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
           )

           + getLineStylesUBODefinition(isSimpleLine) +

           OMMShaderCode(
                   uniform float scalingFactor;
                   uniform float dashingScaleFactor;
                   uniform float timeFrameDeltaSeconds;

                   in vec4 outColor;
                   in float outLengthPrefix;
                   in float outLineSide;
                   flat in int outStylingIndex;
                   out vec4 fragmentColor;

                   void main() {
                       LineStyle style = uLineStyles.lineValues[outStylingIndex];

                       float opacity = style.opacity;

                       if(opacity == 0.0) {
                           discard;
                       }

                       float a = outColor.a * opacity;
                       float aGap = style.gapColorA * opacity;

                       if(style.blur > 0.0) {
                           float scaledWidth = style.width * scalingFactor;
                           float halfScaledWidth = scaledWidth / 2.0;
                           float blur = style.blur * scalingFactor;
                           float lineEdgeDistance = (1.0 - abs(outLineSide)) * halfScaledWidth;
                           float blurAlpha = clamp(lineEdgeDistance / blur, 0.0, 1.0);

                           if(blurAlpha == 0.0) {
                               discard;
                           }

                           a *= blurAlpha;
                           aGap *= blurAlpha;
                       }

                       vec4 mainColor = vec4(outColor.r, outColor.g, outColor.b, 1.0) * a;
                       vec4 gapColor = vec4(style.gapColorR, style.gapColorG, style.gapColorB, 1.0) * aGap;

                       if (style.dotted == 1.0) {
                           float skew = style.dottedSkew;

                           float scaledWidth = style.width * dashingScaleFactor;
                           float halfScaledWidth = scaledWidth / 2.0;
                           float cycleLength = scaledWidth * skew;
                           float timeOffset = timeFrameDeltaSeconds * style.dashAnimationSpeed * scaledWidth;
                           float positionInCycle = mod(outLengthPrefix * skew + timeOffset, 2.0 * cycleLength) / cycleLength;

                           float scalingRatio =  dashingScaleFactor / scalingFactor;
                           vec2 pos = vec2((positionInCycle * 2.0 - 1.0) * scalingRatio, outLineSide);

                           if(dot(pos, pos) >= 1.0) {
                               discard;
                           }
                       } else if(style.numDashValues > 0.0) {
                           float scaledWidth = style.width * dashingScaleFactor;
                           float timeOffset = timeFrameDeltaSeconds * style.dashAnimationSpeed * scaledWidth;
                           float intraDashPos = mod(outLengthPrefix + timeOffset, style.dashArray3 * scaledWidth);

                           float dxt = style.dashArray0 * scaledWidth;
                           float dyt = style.dashArray1 * scaledWidth;
                           float dzt = style.dashArray2 * scaledWidth;
                           float dwt = style.dashArray3 * scaledWidth;

                           if (style.dashFade == 0.0) {
                               if ((intraDashPos > dxt && intraDashPos < dyt) || (intraDashPos > dzt && intraDashPos < dwt)) {
                                   // Simple case without fade
                                   fragmentColor = gapColor;
                                   return;
                               } else {
                                   fragmentColor = mainColor;
                                   return;
                               }
                           } else {
                               if (intraDashPos > dxt && intraDashPos < dyt) {
                                   float relG = (intraDashPos - dxt) / (dyt - dxt);

                                   if(relG < (style.dashFade * 0.5)) {
                                       float wg = relG / (style.dashFade * 0.5);
                                       fragmentColor = gapColor * wg + mainColor * (1.0 - wg);
                                       return;
                                   } else {
                                       fragmentColor = gapColor;
                                       return;
                                   }
                               }

                               if (intraDashPos > dzt && intraDashPos < dwt) {
                                   float relG = (intraDashPos - dzt) / (dwt - dzt);
                                   if (relG < style.dashFade) {
                                       float wg = relG / style.dashFade;
                                       fragmentColor = gapColor * wg + mainColor * (1.0 - wg);
                                       return;
                                   } else if (1.0 - relG < style.dashFade) {
                                       float wg = (1.0 - relG) / style.dashFade;
                                       fragmentColor = gapColor * wg + mainColor * (1.0 - wg);
                                       return;
                                   }
                                   else {
                                       fragmentColor = gapColor;
                                       return;
                                   }
                               }
                           }
                       }

                       if(a == 0.0) {
                           discard;
                       }

                       fragmentColor = mainColor;
                   });
}

std::string ColorLineGroup2dShaderOpenGl::getFragmentShader() {
    if (isSimpleLine) {
        return getSimpleLineFragmentShader();
    } else {
        return getLineFragmentShader();
    }
}
