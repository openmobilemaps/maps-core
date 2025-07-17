/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "SkySphereShaderOpenGl.h"
#include "OpenGlContext.h"

const std::string SkySphereShaderOpenGl::programName = "UBMAP_SkySphereShaderOpenGl";

std::string SkySphereShaderOpenGl::getProgramName() {
    return programName;
}

void SkySphereShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    inverseVPMatrixHandle = glGetUniformLocation(program, "uInverseVPMatrix");
    cameraPositionHandle = glGetUniformLocation(program, "uCameraPosition");
}

std::shared_ptr<ShaderProgramInterface> SkySphereShaderOpenGl::asShaderProgramInterface() {
    return shared_from_this();
}

void SkySphereShaderOpenGl::setCameraProperties(const std::vector<float> &inverseVP,
                                                const Vec3D &cameraPosition) {
    std::lock_guard<std::mutex> lock(dataMutex);
    this->inverseVPMatrix = inverseVP;
    this->cameraPosition = cameraPosition;
}

void SkySphereShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        glUniformMatrix4fv(inverseVPMatrixHandle, 1, false, (GLfloat *)inverseVPMatrix.data());
        glUniform4f(cameraPositionHandle, cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0);
    }
}

std::string SkySphereShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                      uniform mat4 umMatrix;
                                      in vec4 vPosition;
                                      in vec2 texCoordinate;
                                      out vec2 v_screenPos;
                                      out vec2 v_textureScaleFactors;

                                      void main() {
                                          gl_Position = vPosition; // in screen coordinates
                                          v_screenPos = vPosition.xy;
                                          v_textureScaleFactors = abs(texCoordinate) / abs(vPosition.xy);
                                      });
}

std::string SkySphereShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                      precision mediump float;
                                      uniform vec4 uOriginOffset;
                                      uniform vec4 uCameraPosition;
                                      uniform mat4 uInverseVPMatrix;
                                      uniform sampler2D textureSampler;

                                      in vec2 v_screenPos;
                                      in vec2 v_textureScaleFactors;
                                      out vec4 fragmentColor;

                                      const float PI = 3.141592653589793;

                                      void main() {
                                          vec4 posCart = uInverseVPMatrix * vec4(v_screenPos.x, v_screenPos.y, 1.0, 1.0);
                                          posCart /= posCart.w;

                                          // Assume the sky on a sphere around the camera (and not the earth's center)
                                          vec3 dirCamera = normalize(posCart.xyz - uCameraPosition.xyz);

                                          float rasc = atan(dirCamera.z, dirCamera.x) + PI;
                                          float decl = asin(dirCamera.y);

                                          vec2 texCoords = vec2(
                                                  -(rasc / (2.0 * PI)) + 1.0,
                                                  -decl / PI + 0.5
                                          ) * v_textureScaleFactors;

                                          // Mutli-sampling
                                          /*ivec2 textureSize = textureSize(textureSampler, 0);
                                          vec2 size = vec2(1.0 / float(textureSize.x), 1.0 / float(textureSize.y));
                                          size.y *= cos(decl);

                                          fragmentColor = 0.2837 * texture(textureSampler, texCoords);
                                          fragmentColor = fragmentColor + 0.179083648 * texture(textureSampler, texCoords + (vec2(0.0, -2.0)) * size);
                                          fragmentColor = fragmentColor + 0.179083648 * texture(textureSampler, texCoords + (vec2(2.0, 0.0)) * size);
                                          fragmentColor = fragmentColor + 0.179083648 * texture(textureSampler, texCoords + (vec2(0.0, 2.0)) * size);
                                          fragmentColor = fragmentColor + 0.179083648 * texture(textureSampler, texCoords + (vec2(-2.0, 0.0)) * size);*/

                                          // Single sample
                                          fragmentColor = texture(textureSampler, texCoords);

                                          // Debug rendering
                                          /*if (abs(mod(texCoords.x, 0.125)) < 0.001) {
                                              fragmentColor = vec4(0.0, 0.0, 1.0, 1.0);
                                          } else if (abs(mod(texCoords.y, 0.125)) < 0.001) {
                                              fragmentColor = vec4(0.0, 1.0, 1.0, 1.0);
                                          } else {
                                              fragmentColor = vec4(texCoords, 0.0, 1.0);
                                          }*/
                                      }
    );
}
