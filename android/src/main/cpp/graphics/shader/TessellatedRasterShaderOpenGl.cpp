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
#include "TessellationSettings.h"

TessellatedRasterShaderOpenGl::TessellatedRasterShaderOpenGl(bool projectOntoUnitSphere)
        : RasterShaderOpenGl(projectOntoUnitSphere ? "UBMAP_TessellatedRasterShaderUnitSphereOpenGl" : "UBMAP_TessellatedRasterShaderOpenGl")
        , texturedPolygonProgramName(projectOntoUnitSphere ? "UBMAP_TessellatedTexturedPolygonRasterShaderUnitSphereOpenGl"
                                                           : "UBMAP_TessellatedTexturedPolygonRasterShaderOpenGl")
{}

TessellatedRasterShaderOpenGl::TessellatedRasterShaderOpenGl(const std::string &programName)
        : RasterShaderOpenGl(programName)
        , texturedPolygonProgramName(programName + "TexturedPolygon")
{}

std::string TessellatedRasterShaderOpenGl::getProgramName() {
    return texturedPolygonMode ? texturedPolygonProgramName : RasterShaderOpenGl::getProgramName();
}

void TessellatedRasterShaderOpenGl::setTexturedPolygonMode() {
    texturedPolygonMode = true;
}

void TessellatedRasterShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int controlShader = loadShader(GL_TESS_CONTROL_SHADER, getControlShader());
    int evaluationShader = loadShader(GL_TESS_EVALUATION_SHADER, getEvaluationShader());

#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
    int geometryShader = loadShader(GL_GEOMETRY_SHADER, getGeometryShader());
#endif

    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glAttachShader(program, controlShader);
    glDeleteShader(controlShader);
    glAttachShader(program, evaluationShader);
    glDeleteShader(evaluationShader);
    glAttachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
    glAttachShader(program, geometryShader);
    glDeleteShader(geometryShader);
#endif

    glLinkProgram(program);

    checkGlProgramLinking(program);

    openGlContext->storeProgram(getProgramName(), program);
}

std::string TessellatedRasterShaderOpenGl::getVertexShader() {
    if (texturedPolygonMode) {
        return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        in vec4 vPosition;
                                        in vec2 vFrameCoord;
                                        in vec2 texCoordinate;
                                        in float vSkirtOffset;
                                        out vec2 c_framecoord;
                                        out vec2 c_texcoord;
                                        out float c_skirtOffset;

                                        void main() {
                                            gl_Position = vPosition;
                                            c_framecoord = vFrameCoord;
                                            c_texcoord = texCoordinate;
                                            c_skirtOffset = vSkirtOffset;
                                        }
        );
    }

    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        in vec4 vPosition;
                                        in vec2 vFrameCoord;
                                        in vec2 texCoordinate;
                                        out vec2 c_framecoord;
                                        out vec2 c_texcoord;

                                        void main() {
                                            gl_Position = vPosition;
                                            c_framecoord = vFrameCoord;
                                            c_texcoord = texCoordinate;
                                        }
    );
}

std::string TessellatedRasterShaderOpenGl::getControlShader() {
    if (texturedPolygonMode) {
        return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        layout (vertices = 3) out;

                                        uniform int uSubdivisionFactor;

                                        in vec2 c_framecoord[];
                                        in vec2 c_texcoord[];
                                        in float c_skirtOffset[];
                                        out vec2 e_framecoord[];
                                        out vec2 e_texcoord[];
                                        out float e_skirtOffset[];

                                        void main()
                                        {
                                            gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
                                            e_framecoord[gl_InvocationID] = c_framecoord[gl_InvocationID];
                                            e_texcoord[gl_InvocationID] = c_texcoord[gl_InvocationID];
                                            e_skirtOffset[gl_InvocationID] = c_skirtOffset[gl_InvocationID];

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

    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        layout (vertices = 4) out;

                                        uniform int uSubdivisionFactor;

                                        in vec2 c_framecoord[];
                                        in vec2 c_texcoord[];
                                        out vec2 e_framecoord[];
                                        out vec2 e_texcoord[];

                                        void main()
                                        {
                                            gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
                                            e_framecoord[gl_InvocationID] = c_framecoord[gl_InvocationID];
                                            e_texcoord[gl_InvocationID] = c_texcoord[gl_InvocationID];

                                            if (gl_InvocationID == 0)
                                            {
                                                float tessFactor = float(uSubdivisionFactor);

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
    if (texturedPolygonMode) {
        return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        layout(triangles, equal_spacing, ccw) in;

                                        uniform mat4 umMatrix;
                                        uniform vec4 uOriginOffset;
                                        uniform vec4 uOrigin;
                                        uniform bool uIs3d;
                                        uniform bool uHasElevationTexture;
                                        uniform sampler2D elevationTextureSampler;

                                        in vec2 e_framecoord[];
                                        in vec2 e_texcoord[];
                                        in float e_skirtOffset[];
#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
                                        out vec2 g_texcoord;
#else
                                        out vec2 v_texcoord;
#endif
                                        const float BlendScale = 100.0;
                                        const float BlendOffset = 0.01;
                                        const float ElevationOffset = 8.4 * 1000.0 * 1000.0;
                                        const float ElevationScale = 1.0 / (1.5 * 1000.0 * 1000.0 * 1000.0);

                                        vec2 baryinterp(vec2 c0, vec2 c1, vec2 c2, vec3 bary) {
                                            return c0 * bary.x + c1 * bary.y + c2 * bary.z;
                                        }

                                        float baryinterp(float c0, float c1, float c2, vec3 bary) {
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

                                        float decodeElevation(vec3 rgbSample) {
                                            return rgbSample.r * 256.0 * 256.0 * 255.0 +
                                                   rgbSample.g * 256.0 * 255.0 +
                                                   rgbSample.b * 255.0;
                                        }

                                        void main() {
                                            vec3 bary = gl_TessCoord.xyz;

                                            vec4 position = baryinterp(gl_in[0].gl_Position,
                                                                       gl_in[1].gl_Position,
                                                                       gl_in[2].gl_Position,
                                                                       bary);

                                            if (uIs3d) {
                                                vec2 frameCoord = baryinterp(e_framecoord[0],
                                                                             e_framecoord[1],
                                                                             e_framecoord[2],
                                                                             bary);

                                                vec4 bent = transform(frameCoord, uOrigin) - uOriginOffset;
                                                float blend = clamp(length(uOriginOffset) * BlendScale - BlendOffset, 0.0, 1.0);
                                                position = mix(position, bent, blend);
                                            }

                                            vec2 texCoord = baryinterp(e_texcoord[0],
                                                                       e_texcoord[1],
                                                                       e_texcoord[2],
                                                                       bary);

                                            if (uIs3d && uHasElevationTexture) {
                                                vec3 surfaceNormal = normalize(position.xyz + uOrigin.xyz + uOriginOffset.xyz);
                                                vec3 elevationSample = texture(elevationTextureSampler, texCoord).rgb;
                                                float elevation = (decodeElevation(elevationSample) - ElevationOffset) * ElevationScale;
                                                float skirtOffset = baryinterp(e_skirtOffset[0], e_skirtOffset[1], e_skirtOffset[2], bary);
                                                position.xyz += surfaceNormal * (elevation - skirtOffset);
                                            }

                                            gl_Position = uFrameUniforms.vpMatrix * ((umMatrix * vec4(position.xyz, 1.0)) + uOriginOffset);
#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
                                            g_texcoord = texCoord;
#else
                                            v_texcoord = texCoord;
#endif
                                        }
   );
    }

    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        layout(quads, equal_spacing, cw) in;

                                        uniform mat4 umMatrix;
                                        uniform vec4 uOriginOffset;
                                        uniform vec4 uOrigin;
                                        uniform bool uIs3d;

                                        in vec2 e_framecoord[];
                                        in vec2 e_texcoord[];
#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
                                        out vec2 g_texcoord;
#else
                                        out vec2 v_texcoord;
#endif
                                        const float BlendScale = 100.0;
                                        const float BlendOffset = 0.01;

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

                                        vec4 transform(vec2 coordinate, vec4 origin) {
                                            float x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x;
                                            float y = 1.0 * cos(coordinate.y) - origin.y;
                                            float z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z;
                                            return vec4(x, y, z, 0.0);
                                        }

                                        void main() {
                                            vec2 uv = gl_TessCoord.xy;

                                            vec4 p00 = gl_in[0].gl_Position;
                                            vec4 p01 = gl_in[1].gl_Position;
                                            vec4 p10 = gl_in[2].gl_Position;
                                            vec4 p11 = gl_in[3].gl_Position;
                                            vec4 position = bilerp(p00, p01, p10, p11, uv);

                                            if (uIs3d) {
                                                vec2 f00 = e_framecoord[0];
                                                vec2 f01 = e_framecoord[1];
                                                vec2 f10 = e_framecoord[2];
                                                vec2 f11 = e_framecoord[3];
                                                vec2 frameCoord = bilerp(f00, f01, f10, f11, uv);

                                                vec4 bent = transform(frameCoord, uOrigin) - uOriginOffset;
                                                float blend = clamp(length(uOriginOffset) * BlendScale - BlendOffset, 0.0, 1.0);
                                                position = mix(position, bent, blend);
                                            }

                                            vec2 t00 = e_texcoord[0];
                                            vec2 t01 = e_texcoord[1];
                                            vec2 t10 = e_texcoord[2];
                                            vec2 t11 = e_texcoord[3];
                                            vec2 texCoord = bilerp(t00, t01, t10, t11, uv);

                                            gl_Position = uFrameUniforms.vpMatrix * ((umMatrix * vec4(position.xyz, 1.0)) + uOriginOffset);
#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
                                            g_texcoord = texCoord;
#else
                                            v_texcoord = texCoord;
#endif
                                        }
   );
}

#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
std::string TessellatedRasterShaderOpenGl::getGeometryShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        layout(triangles) in;
                                        layout(line_strip, max_vertices = 6) out;

                                        in vec2 g_texcoord[];
                                        out vec2 v_texcoord;

                                        void edgeLine(int a, int b) {
                                            gl_Position = gl_in[a].gl_Position;
                                            v_texcoord = g_texcoord[a];
                                            EmitVertex();

                                            gl_Position = gl_in[b].gl_Position;
                                            v_texcoord = g_texcoord[b];
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
#endif
