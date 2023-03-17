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
                 texture2d<float> texture0 [[ texture(0)]],
                 sampler textureSampler [[sampler(0)]]
                 )
{
  // Compute tesselated xy + uv

  float2 p0 = patch[0].position;
  float2 p1 = patch[1].position;
  float2 p2 = patch[2].position;
  float2 pos = baryinterp(p0, p1, p2, positionInPatch);

  float2 uv0 = patch[0].uv;
  float2 uv1 = patch[1].uv;
  float2 uv2 = patch[2].uv;
  float2 uv = baryinterp(uv0, uv1, uv2, positionInPatch);


  float4 color = texture0.sample(textureSampler, uv);
  float height = -10000 + ((color.r * 256 * 256 * 256 + color.g * 256 * 256 + color.b * 256) * 0.1);

  // xy -> xyz (globe)

  float px = pos.x;
  float py = pos.y;
  float R = 6371000;

  // longitude [0, 2pi]
  float lambda = px / R;

  // latitude, [0, pi] statt [-90, 90]
  float phi = atan(sinh(py / R)) + 3.1415926 / 2.0;

  float radius = 1.0 + (height / R) * 1.0;

  float4 pos3d = float4(radius*sin(phi)*cos(lambda+time*0.0),
                           radius*cos(-phi),
                           radius*sin(-phi)*sin(lambda+time*0.0),
                           1);

  // projection to screen

  // float4 position = mvpMatrix * float4(pos.xy, 0.0, 1.0);
  float4 position = pos3d; // TODO: Apply mvpMatrix

  VertexOut out {
    .position = mvpMatrix * position,
    .uv = uv
  };

  return out;
}



