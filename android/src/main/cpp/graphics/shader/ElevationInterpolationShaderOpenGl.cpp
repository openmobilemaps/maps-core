/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ElevationInterpolationShaderOpenGl.h"

ElevationInterpolationShaderOpenGl::ElevationInterpolationShaderOpenGl() : programName("UBMAP_ElevationShaderOpengl") {}

std::string ElevationInterpolationShaderOpenGl::getProgramName() {
    return programName;
}

void ElevationInterpolationShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    checkGlProgramLinking(program);

    openGlContext->storeProgram(programName, program);
}

std::shared_ptr<ShaderProgramInterface> ElevationInterpolationShaderOpenGl::asShaderProgramInterface() {
    return shared_from_this();
}

std::string ElevationInterpolationShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                              precision highp int;
                                              precision highp sampler2D;
                                              uniform sampler2D textureSampler;
                                              in vec2 v_texcoord;
                                              out vec4 fragmentColor;

                                              // Decodes elevation in meters from an RGBA-encoded pixel.
                                              // Matches the provided formula: ((r*256*256*255) + (g*256*255) + (b*255))/10 - 10000
                                              float decodeElevation(in vec4 rgbaAltitude)
                                              {
                                                  return (rgbaAltitude.r * 256.0 * 256.0 * 255.0 +
                                                          rgbaAltitude.g * 256.0 * 255.0 +
                                                          rgbaAltitude.b * 255.0) / 10.0 - 10000.0;
                                              }

                                              // Encodes elevation in meters back into RGB using the inverse of the above scheme.
                                              // We map elevation to a 24-bit integer value V in range [0, 256*256*256-1] where
                                              // elevationMeters = V/10 - 10000  =>  V = (elevationMeters + 10000) * 10
                                              vec4 encodeElevation(in float elevationMeters)
                                              {
                                                  // Clamp to representable range [0, 16777215]
                                                  float value = (elevationMeters + 10000.0) * 10.0;
                                                  value = clamp(value, 0.0, 16777215.0);

                                                  // Convert to 24-bit integer and split across R,G,B components.
                                                  int intValue = int(value);
                                                  int r = (intValue / (256 * 256)) & 255;
                                                  int g = (intValue / 256) & 255;
                                                  int b = intValue & 255;

                                                  // Normalize to [0,1] range for storage in 8-bit per channel texture.
                                                  vec4 outColor = vec4(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0, 1.0);
                                                  return outColor;
                                              }

                                              // Simple bilinear sampling helper that decodes elevation from four neighbors and blends.
                                              float bilinearSampleElevation(in sampler2D textureSampler,
                                                                            in highp vec2 uv)
                                              {
                                                  // Sample the four neighboring texels around the target coordinate.
                                                  vec2 texSize = vec2(textureSize(textureSampler, 0));
                                                  vec2 coord = uv * texSize - 0.5; // shift so integer coords land at texel centers

                                                  vec2 base = floor(coord);
                                                  vec2 f = coord - base; // fractional part for weights

                                                  vec2 uv00 = (base + vec2(0.0, 0.0) + 0.5) / texSize;
                                                  vec2 uv10 = (base + vec2(1.0, 0.0) + 0.5) / texSize;
                                                  vec2 uv01 = (base + vec2(0.0, 1.0) + 0.5) / texSize;
                                                  vec2 uv11 = (base + vec2(1.0, 1.0) + 0.5) / texSize;

                                                  vec4 c00 = texture(textureSampler, uv00);
                                                  vec4 c10 = texture(textureSampler, uv10);
                                                  vec4 c01 = texture(textureSampler, uv01);
                                                  vec4 c11 = texture(textureSampler, uv11);

                                                  float e00 = decodeElevation(c00);
                                                  float e10 = decodeElevation(c10);
                                                  float e01 = decodeElevation(c01);
                                                  float e11 = decodeElevation(c11);

                                                  // Bilinear interpolation
                                                  float e0 = mix(e00, e10, f.x);
                                                  float e1 = mix(e01, e11, f.x);
                                                  float e = mix(e0, e1, f.y);

                                                  return e;
                                              }

                                              void main()
                                              {
                                                  float elevation = bilinearSampleElevation(textureSampler, v_texcoord);
                                                  fragmentColor = encodeElevation(elevation);
                                              }
    );
}
