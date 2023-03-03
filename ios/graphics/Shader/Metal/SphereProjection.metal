//
//  SphereProjection.metal
//  Meteo
//
//  Created by Nicolas Märki on 03.03.23.
//

#include <metal_stdlib>
#include "DataStructures.metal"
using namespace metal;

float2 baryinterp(float2 a, float2 b, float2 c, float3 p) {
  return a * p.x + b * p.y + c * p.z;
}

[[patch(triangle, 3)]]
vertex VertexOut
sphereProjectionVertexShader(const patch_control_point<VertexIn> patch [[stage_in]],
                 const float3 positionInPatch [[position_in_patch]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &time [[buffer(2)]])
{
  float2 p0 = patch[0].position;
  float2 p1 = patch[1].position;
  float2 p2 = patch[3].position;
  float2 pos = baryinterp(p0, p1, p2, positionInPatch);

  float2 uv0 = patch[0].uv;
  float2 uv1 = patch[1].uv;
  float2 uv2 = patch[3].uv;
  float2 uv = baryinterp(uv0, uv1, uv2, positionInPatch);

  float px = pos.x;
  float py = pos.y;

  float R = 6371000;
  float lambda = px / R;
  float phi = 2*atan(exp(py / R)) - 0 * 3.1415926 / 2;

  float radius = 1.0;
  float ratio = 2556.0/1179.0;

  VertexOut out {
    .position = float4(radius*sin(phi)*cos(lambda+time), radius*cos(phi) / ratio, radius*sin(phi)*sin(lambda+time), 1),
    .uv = uv
  };



  return out;
}

kernel void compute_tess_factors(
     device MTLTriangleTessellationFactorsHalf *factors [[buffer(0)]],
     uint pid [[thread_position_in_grid]]) {
       factors[pid].edgeTessellationFactor[0] = 2;
       factors[pid].edgeTessellationFactor[1] = 2;
       factors[pid].edgeTessellationFactor[2] = 2;
       factors[pid].insideTessellationFactor = 2;
     }

