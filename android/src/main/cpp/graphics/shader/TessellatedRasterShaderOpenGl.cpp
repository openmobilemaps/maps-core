/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TessellatedRasterShaderOpenGl.h"
#include "OpenGlContext.h"

TessellatedRasterShaderOpenGl::TessellatedRasterShaderOpenGl(bool projectOntoUnitSphere)
        : RasterShaderOpenGl(projectOntoUnitSphere ? "UBMAP_TessellatedRasterShaderUnitSphereOpenGl" : "UBMAP_TessellatedRasterShaderOpenGl")
{}

void TessellatedRasterShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int controlShader = loadShader(GL_TESS_CONTROL_SHADER, getControlShader());
    int evalutationShader = loadShader(GL_TESS_EVALUATION_SHADER, getEvaluationShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glAttachShader(program, controlShader);
    glDeleteShader(controlShader);
    glAttachShader(program, evalutationShader);
    glDeleteShader(evalutationShader);
    glAttachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);
    glLinkProgram(program);

    checkGlProgramLinking(program);

    openGlContext->storeProgram(RasterShaderOpenGl::getProgramName(), program);
}

std::string TessellatedRasterShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        in vec4 vPosition;
                                        in vec2 texCoordinate;
                                        out vec2 c_texcoord;

                                        void main() {
                                            gl_Position = vPosition;
                                            c_texcoord = texCoordinate;
                                        }
    );
}

std::string TessellatedRasterShaderOpenGl::getControlShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        layout (vertices = 4) out;

                                        uniform int uSubdivisionFactor;

                                        in vec2 c_texcoord[];
                                        out vec2 e_texcoord[];

                                        void main()
                                        {
                                            gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
                                            e_texcoord[gl_InvocationID] = c_texcoord[gl_InvocationID];

                                            if (gl_InvocationID == 0)
                                            {
                                                float tessFactor = 16.0; //float(uSubdivisionFactor);

                                                gl_TessLevelOuter[0] = tessFactor;
                                                gl_TessLevelOuter[1] = tessFactor;
                                                gl_TessLevelOuter[2] = tessFactor;
                                                gl_TessLevelOuter[3] = tessFactor;

                                                gl_TessLevelInner[0] = tessFactor;
                                                gl_TessLevelInner[1] = tessFactor;
                                            }
                                        }
           );
}


std::string TessellatedRasterShaderOpenGl::getEvaluationShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        layout(quads, fractional_odd_spacing, cw) in;

                                        uniform mat4 umMatrix;
                                        uniform vec4 uOriginOffset;

                                        in vec2 e_texcoord[];
                                        out vec2 v_texcoord;

                                        vec2 bilerp(vec2 c00, vec2 c01, vec2 c10, vec2 c11, vec2 uv) {
                                            vec2 c0 = mix(c00, c01, uv.x);
                                            vec2 c1 = mix(c10, c11, uv.x);
                                            return mix(c0, c1, uv.y);
                                        }

                                        vec4 bilerp(vec4 c00, vec4 c01, vec4 c10, vec4 c11, vec2 uv) {
                                            vec4 c0 = mix(c00, c01, uv.x);
                                            vec4 c1 = mix(c10, c11, uv.x);
                                            return mix(c0, c1, uv.y);
                                        }

                                        void main() {
                                            vec2 uv = gl_TessCoord.xy;

                                            vec4 p00 = gl_in[0].gl_Position;
                                            vec4 p01 = gl_in[1].gl_Position;
                                            vec4 p10 = gl_in[2].gl_Position;
                                            vec4 p11 = gl_in[3].gl_Position;

                                            vec4 position = bilerp(p00, p01, p10, p11, uv);
                                            gl_Position = uFrameUniforms.vpMatrix * ((umMatrix * position) + uOriginOffset);

                                            vec2 t00 = e_texcoord[0];
                                            vec2 t01 = e_texcoord[1];
                                            vec2 t10 = e_texcoord[2];
                                            vec2 t11 = e_texcoord[3];

                                            v_texcoord = bilerp(t00, t01, t10, t11, uv);
                                        }
   );
}

