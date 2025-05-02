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
        // maximum number of polygonStyles and numStyles, padded to 16 byte alignment
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
    if (isSimpleLine) {
        return OMMVersionedGlesShaderCode(320 es,
                                          precision highp float;
                                                  uniform mat4 umMatrix;
                                                  uniform mat4 uvpMatrix;
                                                  uniform vec4 uOriginOffset;
               ) + (projectOntoUnitSphere ?
                   OMMShaderCode(
                           uniform vec4 uLineOrigin;
                           in vec3 vPointA;
                           in vec3 vPointB;
                   ) : OMMShaderCode(
                           in vec2 vPointA;
                           in vec2 vPointB;
               )) + OMMShaderCode(
                       in float vVertexIndex;
                       in float vStyleInfo;

                       ) + getLineStylesUBODefinition(isSimpleLine) + OMMShaderCode(

                       uniform float scaleFactor;
                       uniform float dashingScaleFactor;
                       flat out int lineIndex;
                       out float radius;
                       out float fSegmentType;
                       out vec3 pointDeltaA;
                       out vec3 pointBDeltaA;
                       out vec4 color;

                       void main() {
                           float fStyleIndex = mod(vStyleInfo, 256.0);
                           lineIndex = clamp(int(floor(fStyleIndex + 0.5)), 0, uLineStyles.numStyles);
                           float width = uLineStyles.lineValues[lineIndex].width;
                           float isScaled = uLineStyles.lineValues[lineIndex].widthAsPixels;
                           color = vec4(uLineStyles.lineValues[lineIndex].colorR, uLineStyles.lineValues[lineIndex].colorG, uLineStyles.lineValues[lineIndex].colorB,
                                        uLineStyles.lineValues[lineIndex].colorA);
                           fSegmentType = vStyleInfo / 256.0;

               ) + (projectOntoUnitSphere ? OMMShaderCode(
                vec3 vertexPosition = vPointB;
                vec3 lengthNormal = normalize(vPointB - vPointA);
        ) : OMMShaderCode(
                            vec2 vertexPosition = vPointB;
                            vec3 lengthNormal = vec3(normalize(vPointB - vPointA), 0.0);
                            vec3 radialNormal = vec3(0.0, 0.0, 1.0);
                    )) + OMMShaderCode(

                       float widthNormalFactor = 1.0;
                       float lengthNormalFactor = 1.0;
                       if(vVertexIndex == 0.0) {
                           lengthNormalFactor = -1.0;
                           widthNormalFactor = -1.0;
                           vertexPosition = vPointA;
                       } else if(vVertexIndex == 1.0) {
                           lengthNormalFactor = -1.0;
                           vertexPosition = vPointA;
                       } else if(vVertexIndex == 2.0) {
                           // all fine
                       } else if(vVertexIndex == 3.0) {
                           widthNormalFactor = -1.0;
                       }

           ) + (projectOntoUnitSphere ? OMMShaderCode(
                   vec3 radialNormal = normalize(vertexPosition + uLineOrigin.xyz);
                ) : "")
           + OMMShaderCode(
                   vec3 widthNormal = normalize(cross(radialNormal, lengthNormal));

                   float offsetFloat = 0.0;
                   vec3 offset = vec3(widthNormal * offsetFloat);

                   widthNormal *= widthNormalFactor;
                   lengthNormal *= lengthNormalFactor;

                   float scaledWidth = width * 0.5;
                   if (isScaled > 0.0) {
                       scaledWidth = scaledWidth * scaleFactor;
                   }

                   vec3 displ = uOriginOffset.xyz + vec3(lengthNormal + widthNormal) * vec3(scaledWidth);
                   vec4 extendedPosition = umMatrix *
           ) + (projectOntoUnitSphere ? OMMShaderCode(
                       vec4(vertexPosition, 1.0);
               ) : OMMShaderCode(
                       vec4(vertexPosition, 0.0, 1.0);
               )) + OMMShaderCode(
                       extendedPosition.xyz += displ;
                       gl_Position = uvpMatrix * extendedPosition;
                       radius = scaledWidth;
           ) + (projectOntoUnitSphere ? OMMShaderCode(
                       pointDeltaA = extendedPosition.xyz - ((vPointA + uOriginOffset.xyz));
                       pointBDeltaA = ((vPointB + uOriginOffset.xyz)) - ((vPointA + uOriginOffset.xyz));
               ) : OMMShaderCode(
                       pointDeltaA = extendedPosition.xyz - ((vec3(vPointA, 0.0) + uOriginOffset.xyz));
                       pointBDeltaA = ((vec3(vPointB, 0.0) + uOriginOffset.xyz)) - ((vec3(vPointA, 0.0) + uOriginOffset.xyz));
               )) + OMMShaderCode(
                       }
        );
    } else {
        return OMMVersionedGlesShaderCode(320 es,
                                          precision highp float;
                                                  uniform mat4 umMatrix;
                                                  uniform mat4 uvpMatrix;
                                                  uniform vec4 uOriginOffset;
               ) + (projectOntoUnitSphere ? OMMShaderCode(
                uniform vec4 uLineOrigin;
                in vec3 vPointA;
                in vec3 vPointB;
        ) : OMMShaderCode(
                            in vec2 vPointA;
                            in vec2 vPointB;
                    )) + OMMShaderCode(
                       in float vVertexIndex;
                       in float vSegmentStartLPos;
                       in float vStyleInfo;

                       ) + getLineStylesUBODefinition(isSimpleLine) + OMMShaderCode(

                        uniform float scaleFactor;
                        uniform float dashingScaleFactor;
                        flat out int lineIndex;
                        out float radius;
                        out float segmentStartLPos;
                        out float fSegmentType;
                        out vec3 pointDeltaA;
                        out vec3 pointBDeltaA;
                        out vec4 color;
                        out float dashingSize;
                        out float scaledBlur;

                        void main() {
                            float fStyleIndex = mod(vStyleInfo, 256.0);
                            lineIndex = clamp(int(floor(fStyleIndex + 0.5)), 0, uLineStyles.numStyles);
                            float width = uLineStyles.lineValues[lineIndex].width;
                            float isScaled = uLineStyles.lineValues[lineIndex].widthAsPixels;
                            float blur = uLineStyles.lineValues[lineIndex].blur;
                            color = vec4(uLineStyles.lineValues[lineIndex].colorR, uLineStyles.lineValues[lineIndex].colorG, uLineStyles.lineValues[lineIndex].colorB,
                                         uLineStyles.lineValues[lineIndex].colorA);
                            segmentStartLPos = vSegmentStartLPos;
                            fSegmentType = vStyleInfo / 256.0;

               ) + (projectOntoUnitSphere ? OMMShaderCode(
                vec3 vertexPosition = vPointB;
                vec3 lengthNormal = normalize(vPointB - vPointA);
        ) : OMMShaderCode(
                            vec2 vertexPosition = vPointB;
                            vec3 lengthNormal = vec3(normalize(vPointB - vPointA), 0.0);
                            vec3 radialNormal = vec3(0.0, 0.0, 1.0);
                    )) + OMMShaderCode(

                       float widthNormalFactor = 1.0;
                       float lengthNormalFactor = 1.0;
                       if(vVertexIndex == 0.0) {
                           lengthNormalFactor = -1.0;
                           widthNormalFactor = -1.0;
                           vertexPosition = vPointA;
                       } else if(vVertexIndex == 1.0) {
                           lengthNormalFactor = -1.0;
                           vertexPosition = vPointA;
                       } else if(vVertexIndex == 2.0) {
                           // all fine
                       } else if(vVertexIndex == 3.0) {
                           widthNormalFactor = -1.0;
                       }

               ) + (projectOntoUnitSphere ? OMMShaderCode(
                vec3 radialNormal = normalize(vertexPosition + uLineOrigin.xyz);
        ) : "") + OMMShaderCode(
                       vec3 widthNormal = normalize(cross(radialNormal, lengthNormal));

                       float offsetFloat = uLineStyles.lineValues[lineIndex].offset * scaleFactor;
                       vec3 offset = vec3(widthNormal * offsetFloat);

                       widthNormal *= widthNormalFactor;
                       lengthNormal *= lengthNormalFactor;

                       float scaledWidth = width * 0.5;
                       dashingSize = width;
                       if (isScaled > 0.0) {
                           scaledWidth = scaledWidth * scaleFactor;
                           blur = blur * scaleFactor;
                           dashingSize *= dashingScaleFactor;
                       }

                       vec3 displ = uOriginOffset.xyz + vec3(lengthNormal + widthNormal) * vec3(scaledWidth) + offset;
                       vec4 extendedPosition = umMatrix *
               ) + (projectOntoUnitSphere ? OMMShaderCode(
                vec4(vertexPosition, 1.0);
        ) : OMMShaderCode(
                            vec4(vertexPosition, 0.0, 1.0);
                    )) + OMMShaderCode(
                       extendedPosition.xyz += displ;
                       gl_Position = uvpMatrix * extendedPosition;

                       radius = scaledWidth;
                       scaledBlur = blur;
               ) + (projectOntoUnitSphere ? OMMShaderCode(
                pointDeltaA = extendedPosition.xyz - ((vPointA + uOriginOffset.xyz) + offset.xyz);
                pointBDeltaA = ((vPointB + uOriginOffset.xyz) + offset.xyz) - ((vPointA + uOriginOffset.xyz) + offset.xyz);
        ) : OMMShaderCode(
                            pointDeltaA = extendedPosition.xyz - ((vec3(vPointA, 0.0) + uOriginOffset.xyz) + offset.xyz);
                            pointBDeltaA = ((vec3(vPointB, 0.0) + uOriginOffset.xyz) + offset.xyz) - ((vec3(vPointA, 0.0) + uOriginOffset.xyz) + offset.xyz);
                    )) + OMMShaderCode(
               }
               );
    }
}

std::string ColorLineGroup2dShaderOpenGl::getFragmentShader() {
    if (isSimpleLine) {
        return OMMVersionedGlesShaderCode(320 es,
                                          precision highp float;

                                          ) + getLineStylesUBODefinition(isSimpleLine) + OMMShaderCode(

                                        flat in int lineIndex;
                                        in float radius;
                                        in float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in line), 2: line end segment, 3: start and end in segment
                                        in vec3 pointDeltaA;
                                        in vec3 pointBDeltaA;
                                        in vec4 color;

                                        out vec4 fragmentColor;

                               void main() {
                                        int segmentType = int(floor(fSegmentType + 0.5));
                                        // 0: butt, 1: round, 2: square
                                        int iCapType = int(floor(uLineStyles.lineValues[lineIndex].capType + 0.5));
                                        float lineLength = length(pointBDeltaA);
                                        float t = dot(pointDeltaA, normalize(pointBDeltaA)) / lineLength;

                                        float d;
                                        if (t < 0.0 || t > 1.0) {
                                                if (segmentType == 0 || iCapType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
                                                d = min(length(pointDeltaA), length(pointDeltaA - pointBDeltaA));
                                        } else if (iCapType == 2) {
                                                float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
                                                vec3 intersectPt = t * pointBDeltaA;
                                                float dOrth = abs(length(pointDeltaA - intersectPt));
                                                d = max(dLen, dOrth);
                                        } else {
                                                discard;
                                        }
                                        } else {
                                                vec3 intersectPt = t * pointBDeltaA;
                                                d = abs(length(pointDeltaA - intersectPt));
                                        }

                                        if (d > radius) {
                                                discard;
                                        }

                                        vec4 fragColor = color;
                                        float opacity = uLineStyles.lineValues[lineIndex].opacity;
                                        float colorA = uLineStyles.lineValues[lineIndex].colorA;

                                        fragmentColor = fragColor;
                                        fragmentColor.a = 1.0;
                                        fragmentColor *= fragColor.a * opacity;
                                });
    } else {
        return OMMVersionedGlesShaderCode(320 es,
                                          precision highp float;

                                          ) + getLineStylesUBODefinition(isSimpleLine) + OMMShaderCode(

                                          uniform float timeFrameDeltaSeconds;
                                          flat in int lineIndex;
                                          in float radius;
                                          in float segmentStartLPos;
                                          in float scaledBlur;
                                          in float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in line), 2: line end segment, 3: start and end in segment
                                          in float dashingSize;
                                          in vec3 pointDeltaA;
                                          in vec3 pointBDeltaA;
                                          in vec4 color;

                                          out vec4 fragmentColor;

                                          void main() {
                                              int segmentType = int(floor(fSegmentType + 0.5));
                                              // 0: butt, 1: round, 2: square
                                              int iCapType = int(floor(uLineStyles.lineValues[lineIndex].capType + 0.5));
                                              float lineLength = length(pointBDeltaA);
                                              float t = dot(pointDeltaA, normalize(pointBDeltaA)) / lineLength;

                                              // dash values: {int numDashInfo, vec4 dashArray} -> stride = 5
                                              int numDashInfos = int(floor(uLineStyles.lineValues[lineIndex].numDashValues + 0.5));

                                              float d;
                                              if (t < 0.0 || t > 1.0) {
                                                  if (numDashInfos > 0 && uLineStyles.lineValues[lineIndex].dashArray0 < 1.0 && uLineStyles.lineValues[lineIndex].dashArray0 > 0.0) {
                                                      discard;
                                                  }
                                                  if (segmentType == 0 || iCapType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
                                                      d = min(length(pointDeltaA), length(pointDeltaA - pointBDeltaA));
                                                  } else if (iCapType == 2) {
                                                      float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
                                                      vec3 intersectPt = t * pointBDeltaA;
                                                      float dOrth = abs(length(pointDeltaA - intersectPt));
                                                      d = max(dLen, dOrth);
                                                  } else {
                                                      discard;
                                                  }
                                              } else {
                                                  vec3 intersectPt = t * pointBDeltaA;
                                                  d = abs(length(pointDeltaA - intersectPt));
                                              }

                                              if (d > radius) {
                                                  discard;
                                              }

                                              vec4 fragColor = color;
                                              float opacity = uLineStyles.lineValues[lineIndex].opacity;
                                              float colorA = uLineStyles.lineValues[lineIndex].colorA;
                                              float colorAGap = uLineStyles.lineValues[lineIndex].gapColorA;

                                              float a = colorA * opacity;
                                              float aGap = colorAGap * opacity;

                                              if (scaledBlur > 0.0 && t > 0.0 && t < 1.0) {
                                                  float nonBlurRange = radius - scaledBlur;
                                                  if (d > nonBlurRange) {
                                                      opacity *= clamp(1.0 - max(0.0, d - nonBlurRange) / scaledBlur, 0.0, 1.0);
                                                  }
                                              }

                                              int iDottedLine = int(floor(uLineStyles.lineValues[lineIndex].dotted + 0.5));

                                              if (iDottedLine == 1) {
                                                  float skew = uLineStyles.lineValues[lineIndex].dottedSkew;

                                                  float factorToT = (radius * 2.0) / lineLength * skew;
                                                  float dashOffset = (radius - skew * radius) / lineLength;

                                                  float dashTotalDotted = 2.0 * factorToT;
                                                  float offset = segmentStartLPos / lineLength;
                                                  float startOffsetSegmentDotted = mod(offset, dashTotalDotted);
                                                  float pos = t + startOffsetSegmentDotted;

                                                  float intraDashPosDotted = mod(pos, dashTotalDotted);
                                                  if ((intraDashPosDotted > 1.0 * factorToT + dashOffset && intraDashPosDotted < dashTotalDotted - dashOffset) ||
                                                  (length(vec2(min(abs(intraDashPosDotted - 0.5 * factorToT), 0.5 * factorToT + dashTotalDotted - intraDashPosDotted) / (0.5 * factorToT + dashOffset), d / radius)) > 1.0)) {
                                                      discard;
                                                  }
                                              } else if (numDashInfos > 0) {
                                                  float factorToT = (radius * 2.0) / lineLength;
                                                  float dashTotal = uLineStyles.lineValues[lineIndex].dashArray3 * factorToT;
                                                  float startOffsetSegment = mod(segmentStartLPos / lineLength, dashTotal);
                                                  float timeOffset = timeFrameDeltaSeconds * uLineStyles.lineValues[lineIndex].dashAnimationSpeed * factorToT;
                                                  float intraDashPos = mod(t + startOffsetSegment + timeOffset, dashTotal);

                                                  float dashFade = uLineStyles.lineValues[lineIndex].dashFade;

                                                  float dxt = uLineStyles.lineValues[lineIndex].dashArray0 * factorToT; // end dash 1
                                                  float dyt = uLineStyles.lineValues[lineIndex].dashArray1 * factorToT; // end gap 1
                                                  float dzt = uLineStyles.lineValues[lineIndex].dashArray2 * factorToT; // end dash 2
                                                  float dwt = uLineStyles.lineValues[lineIndex].dashArray3 * factorToT; // end gap 2

                                                  /*
                                                  vec4 gapColor = vec4(uLineStyles.lineValues[lineIndex].gapColorR, uLineStyles.lineValues[lineIndex].gapColorG,
                                                                       uLineStyles.lineValues[lineIndex].gapColorB, uLineStyles.lineValues[lineIndex].gapColorA);

                                                  if (dashFade == 0.0 && ((intraDashPos > dxt && intraDashPos < dyt) || (intraDashPos > dzt && intraDashPos < dwt))) {
                                                      // Simple case without fade
                                                      fragColor = gapColor;
                                                  } else {
                                                      float dashHalfFade = dashFade * 0.5;

                                                      if (intraDashPos > dxt && intraDashPos < dyt) {
                                                          // Within gap 1
                                                          float relG = (intraDashPos - dxt) / (dyt - dxt);
                                                          if (relG < dashHalfFade) {
                                                              float wG = relG / dashHalfFade;
                                                              fragColor = mix(color, gapColor, wG);
                                                          } else if (1.0 - relG < dashHalfFade) {
                                                              float wG = (1.0 - relG) / dashHalfFade;
                                                              fragColor = mix(color, gapColor, wG);
                                                          } else {
                                                              fragColor = gapColor;
                                                          }
                                                      }

                                                      if (intraDashPos > dzt && intraDashPos < dwt) {
                                                          // Within gap 2
                                                          float relG = (intraDashPos - dzt) / (dwt - dzt);
                                                          if (relG < dashHalfFade) {
                                                              float wG = relG / dashHalfFade;
                                                              fragColor = mix(color, gapColor, wG);
                                                          } else if (1.0 - relG < dashHalfFade) {
                                                              float wG = (1.0 - relG) / dashHalfFade;
                                                              fragColor = mix(color, gapColor, wG);
                                                          } else {
                                                              fragColor = gapColor;
                                                          }
                                                      }
                                                  }*/

                                                  // Only fade-out (dashFade is ratio of gap that is blurred)
                                                  if ((intraDashPos > dxt && intraDashPos < dyt) || (intraDashPos > dzt && intraDashPos < dwt)) {
                                                      float relG = intraDashPos < dyt
                                                              ? (intraDashPos - dxt) / (dyt - dxt)
                                                              : (intraDashPos - dzt) / (dwt - dzt);
                                                      relG /= dashFade;
                                                      vec4 gapColor = vec4(uLineStyles.lineValues[lineIndex].gapColorR, uLineStyles.lineValues[lineIndex].gapColorG,
                                                                           uLineStyles.lineValues[lineIndex].gapColorB, uLineStyles.lineValues[lineIndex].gapColorA);
                                                      fragColor = mix(color, gapColor, min(relG, 1.0));
                                                  }
                                              }

                                              fragmentColor = fragColor;
                                              fragmentColor.a = 1.0;
                                              fragmentColor *= fragColor.a * opacity;
                                          });
    }
}
