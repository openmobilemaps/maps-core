//
//  SphereProjection.metal
//  Meteo
//
//  Created by Nicolas Märki on 03.03.23.
//

#include <metal_stdlib>
#include "DataStructures.metal"
using namespace metal;

float2 baryinterp2(float2 a, float2 b, float2 c, float3 p) {
  return a * p.x + b * p.y + c * p.z;
}

float3 baryinterp3(float3 a, float3 b, float3 c, float3 p) {
    return a * p.x + b * p.y + c * p.z;
}

kernel void compute_tess_factors(
   device MTLTriangleTessellationFactorsHalf *factors [[buffer(0)]],
   uint pid [[thread_position_in_grid]]) {
     factors[pid].edgeTessellationFactor[0] = 64;
     factors[pid].edgeTessellationFactor[1] = 64;
     factors[pid].edgeTessellationFactor[2] = 64;
     factors[pid].insideTessellationFactor = 64;
   }

[[patch(triangle, 3)]]
vertex VertexOut
sphereProjectionVertexShader(const patch_control_point<VertexIn> patch [[stage_in]],
                 const float3 positionInPatch [[position_in_patch]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &time [[buffer(2)]],
                 constant float &layerOffset [[buffer(3)]],
                 texture2d<float> texture0 [[ texture(0)]],
                 sampler textureSampler [[sampler(0)]]
                 )
{
    // Compute tesselated xy + uv

    float2 p0 = patch[0].position;
    float2 p1 = patch[1].position;
    float2 p2 = patch[2].position;

    if (p0.y >= 85.04) {
        p0.y = 90;
    }
    else if (p0.y <= -85.04) {
        p0.y = -90;
    }
    if (p1.y >= 85.04) {
        p1.y = 90;
    }
    else if (p1.y <= -85.04) {
        p1.y = -90;
    }
    if (p2.y >= 85.04) {
        p2.y = 90;
    }
    else if (p2.y <= -85.04) {
        p2.y = -90;
    }

    float2 pos = baryinterp2(p0, p1, p2, positionInPatch);

    float2 uv0 = patch[0].uv;
    float2 uv1 = patch[1].uv;
    float2 uv2 = patch[2].uv;
    float2 uv = baryinterp2(uv0, uv1, uv2, positionInPatch);


    float4 color = texture0.sample(textureSampler, uv);
    float height = -10000 + ((color.r * 256 * 256 * 256 + color.g * 256 * 256 + color.b * 256) * 0.1);
    height *= color.a;

    float R = 6371000;
    float radius = 1.0 + (height / R) * (1.0 + sin(time+pos.x*10)*0.0);

//    float3 n = pos / length(pos);


    float pi = 3.14159;


    float longitude = pos.x / 180.0 * pi + pi; // [0, 2pi]
    float latitude = pos.y / 90.0 * pi - pi; // [-2pi, 0]


//    if (uv.x == 0) {
//        longitude -= 0.00001;
//    }
//    else if (uv.x == 1) {
//        longitude += 0.00001;
//    }
//    if (uv.y == 0) {
//        latitude += 0.00001;
//    }
//    else if (uv.y == 1) {
//        latitude -= 0.00001;
//    }

    float sinLon = sin(longitude);     // [0, 1, 0, -1, 0]
    float cosLon = cos(longitude);     // [1, 0, -1, 0, 1]
    float sinLatH = sin(latitude / 2); // [0, 1, 0, -1, 0]
    float cosLatH = cos(latitude / 2); // [1, 0, -1, 0, 1]

    float x3D = sinLon * sinLatH;
    float y3D = cosLatH;
    float z3D = cosLon * sinLatH;
    float3 n = float3(x3D, y3D, z3D);
    float3 pos3d = n * radius;


  float4 position = mvpMatrix * float4(pos3d, 1.0);
    float s = 0.8;
    float z = position.z / position.w * s;
    z = z + layerOffset * -s * 0.03;

    position.z = z * position.w;


  VertexOut out {
    .position = position,
    .uv = uv,
    .n = n
  };

  return out;
}

[[patch(triangle, 3)]]
vertex VertexOut
sphereColorVertexShader(const patch_control_point<VertexIn3D> patch [[stage_in]],
                             const float3 positionInPatch [[position_in_patch]],
                             constant float4x4 &mvpMatrix [[buffer(1)]],
                             constant float &time [[buffer(2)]],
                             texture2d<float> texture0 [[ texture(0)]],
                             sampler textureSampler [[sampler(0)]]
                             )
{
    // Compute tesselated xy + uv

    float3 p0 = patch[0].position;
    float3 p1 = patch[1].position;
    float3 p2 = patch[2].position;
    float3 pos = baryinterp3(p0, p1, p2, positionInPatch);

    float2 uv0 = patch[0].uv;
    float2 uv1 = patch[1].uv;
    float2 uv2 = patch[2].uv;
    float2 uv = baryinterp2(uv0, uv1, uv2, positionInPatch);


    float4 color = texture0.sample(textureSampler, uv);
    float height = -10000 + ((color.r * 256 * 256 * 256 + color.g * 256 * 256 + color.b * 256) * 0.1);


    float R = 6371000;
    float radius = 1.0 + (height / R) * 1.0;

    float3 pos3d = pos / length(pos) * radius;

  // projection to screen

  float4 position = mvpMatrix * float4(pos3d, 1.0);

  VertexOut out {

    .position = float4(position.x, position.y, position.w, position.w),
    .uv = uv
  };

  return out;
}


