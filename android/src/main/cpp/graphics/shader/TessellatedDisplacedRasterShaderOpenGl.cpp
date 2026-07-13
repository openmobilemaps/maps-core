/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TessellatedDisplacedRasterShaderOpenGl.h"

TessellatedDisplacedRasterShaderOpenGl::TessellatedDisplacedRasterShaderOpenGl(bool projectOntoUnitSphere)
    : TessellatedRasterShaderOpenGl(projectOntoUnitSphere
                                        ? "UBMAP_TessellatedDisplacedRasterShaderUnitSphereOpenGl"
                                        : "UBMAP_TessellatedDisplacedRasterShaderOpenGl") {}

std::string TessellatedDisplacedRasterShaderOpenGl::getEvaluationShader() {
    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                        layout(quads, equal_spacing, cw) in;

                                        uniform mat4 umMatrix;
                                        uniform vec4 uOriginOffset;
                                        uniform vec4 uOrigin;
                                        uniform bool uIs3d;
                                        uniform bool uHasElevationTexture;
                                        uniform sampler2D elevationTextureSampler;

                                        in vec2 e_framecoord[];
                                        in vec2 e_texcoord[];
#if HARDWARE_TESSELLATION_WIREFRAME_OPENGL
                                        out vec2 g_texcoord;
#else
                                        out vec2 v_texcoord;
#endif
                                        const float BlendScale = 100.0;
                                        const float BlendOffset = 0.01;
                                        const float ElevationOffset = 8.4 * 1000.0 * 1000.0;
                                        const float ElevationScale = 1.0 / (1.5 * 1000.0 * 1000.0 * 1000.0);

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

                                        float decodeElevation(vec3 rgbSample) {
                                            return rgbSample.r * 256.0 * 256.0 * 255.0 +
                                                   rgbSample.g * 256.0 * 255.0 +
                                                   rgbSample.b * 255.0;
                                        }

                                        void main() {
                                            vec2 uv = gl_TessCoord.xy;

                                            vec4 p00 = gl_in[0].gl_Position;
                                            vec4 p01 = gl_in[1].gl_Position;
                                            vec4 p10 = gl_in[2].gl_Position;
                                            vec4 p11 = gl_in[3].gl_Position;
                                            vec4 position = bilerp(p00, p01, p10, p11, uv);

                                            vec2 t00 = e_texcoord[0];
                                            vec2 t01 = e_texcoord[1];
                                            vec2 t10 = e_texcoord[2];
                                            vec2 t11 = e_texcoord[3];
                                            vec2 texCoord = bilerp(t00, t01, t10, t11, uv);

                                            if (uIs3d) {
                                                vec2 f00 = e_framecoord[0];
                                                vec2 f01 = e_framecoord[1];
                                                vec2 f10 = e_framecoord[2];
                                                vec2 f11 = e_framecoord[3];
                                                vec2 frameCoord = bilerp(f00, f01, f10, f11, uv);

                                                vec4 bent = transform(frameCoord, uOrigin) - uOriginOffset;
                                                float blend = clamp(length(uOriginOffset) * BlendScale - BlendOffset, 0.0, 1.0);
                                                position = mix(position, bent, blend);

                                                if (uHasElevationTexture) {
                                                    vec3 surfaceNormal = normalize(position.xyz + uOrigin.xyz + uOriginOffset.xyz);
                                                    vec3 elevationSample = texture(elevationTextureSampler, texCoord).rgb;
                                                    float elevation = (decodeElevation(elevationSample) - ElevationOffset) * ElevationScale;
                                                    position.xyz += surfaceNormal * elevation;
                                                }
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
