/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TessellatedColorShaderOpenGl.h"
#include "OpenGlContext.h"

TessellatedColorShaderOpenGl::TessellatedColorShaderOpenGl(bool projectOntoUnitSphere)
        : ColorShaderOpenGl(projectOntoUnitSphere ? "UBMAP_TessellatedColorShaderUnitSphereOpenGl" : "UBMAP_TessellatedColorShaderOpenGl")
{}

void TessellatedColorShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int controlShader = loadShader(GL_TESS_CONTROL_SHADER, getControlShader());
    int evalutationShader = loadShader(GL_TESS_EVALUATION_SHADER, getEvaluationShader());

    /* WIREFRAME DEBUG */
    //int geometryShader = loadShader(GL_GEOMETRY_SHADER, getGeometryShader());

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

    /* WIREFRAME DEBUG */
    //glAttachShader(program, geometryShader);
    //glDeleteShader(geometryShader);

    glLinkProgram(program);

    checkGlProgramLinking(program);

    openGlContext->storeProgram(TessellatedColorShaderOpenGl::getProgramName(), program);
}

std::string TessellatedColorShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        in vec4 vPosition;
                                        in vec2 vFlatPosition;
                                        out vec2 c_flatposition;

                                        void main() {
                                            gl_Position = vPosition;
                                            c_flatposition = vFlatPosition;
                                        }
    );
}

std::string TessellatedColorShaderOpenGl::getControlShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        layout (vertices = 3) out;

                                        uniform int uSubdivisionFactor;

                                        in vec2 c_flatposition[];
                                        out vec2 e_flatposition[];

                                        void main()
                                        {
                                            gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
                                            e_flatposition[gl_InvocationID] = c_flatposition[gl_InvocationID];

                                            if (gl_InvocationID == 0)
                                            {
                                                float tessFactor = float(uSubdivisionFactor);

                                                gl_TessLevelOuter[0] = tessFactor;
                                                gl_TessLevelOuter[1] = tessFactor;
                                                gl_TessLevelOuter[2] = tessFactor;

                                                gl_TessLevelInner[0] = tessFactor;
                                            }
                                        }
           );
}

std::string TessellatedColorShaderOpenGl::getEvaluationShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        layout(triangles, equal_spacing, ccw) in;

                                        uniform mat4 umMatrix;
                                        uniform vec4 uOriginOffset;
                                        uniform vec4 uOrigin;
                                        uniform bool uIs3d;

                                        in vec2 e_flatposition[];

                                        const float BlendScale = 1000.0;
                                        const float BlendOffset = 0.01;

                                        vec2 baryinterp(vec2 c0, vec2 c1, vec2 c2, vec3 bary) {
                                            return c0 * bary.x + c1 * bary.y + c2 * bary.z;
                                        }

                                        vec4 baryinterp(vec4 c0, vec4 c1, vec4 c2, vec3 bary) {
                                            return c0 * bary.x + c1 * bary.y + c2 * bary.z;
                                        }

                                        vec4 transform(vec2 coordinate, vec4 origin) {
                                            float x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x;
                                            float y = 1.0 * cos(coordinate.y) - origin.y;
                                            float z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z;
                                            return vec4(x, y, z, 0.0);
                                        }

                                        void main() {
                                            vec3 bary = gl_TessCoord.xyz;

                                            vec4 p0 = gl_in[0].gl_Position;
                                            vec4 p1 = gl_in[1].gl_Position;
                                            vec4 p2 = gl_in[2].gl_Position;
                                            vec4 position = baryinterp(p0, p1, p2, bary);

                                            vec2 f0 = e_flatposition[0];
                                            vec2 f1 = e_flatposition[1];
                                            vec2 f2 = e_flatposition[2];
                                            vec2 flatPosition = baryinterp(f0, f1, f2, bary);

                                            if (uIs3d) {
                                                vec4 bent = transform(flatPosition, uOrigin) - uOriginOffset;
                                                float blend = clamp(length(uOriginOffset) * BlendScale - BlendOffset, 0.0, 1.0);
                                                position = mix(position, bent, blend);
                                            }

                                            gl_Position = uFrameUniforms.vpMatrix * ((umMatrix * vec4(position.xyz, 1.0)) + uOriginOffset);
                                        }
   );
}

/* WIREFRAME DEBUG */
std::string TessellatedColorShaderOpenGl::getGeometryShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                        layout(triangles) in;
                                        layout(line_strip, max_vertices = 6) out;

                                        void edgeLine(int a, int b) {
                                            gl_Position = gl_in[a].gl_Position;
                                            EmitVertex();

                                            gl_Position = gl_in[b].gl_Position;
                                            EmitVertex();

                                            EndPrimitive();
                                        }

                                        void main() {
                                            edgeLine(0, 1);
                                            edgeLine(1, 2);
                                            edgeLine(2, 0);
                                        }
    );
}
