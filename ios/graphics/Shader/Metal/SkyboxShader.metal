//
//  SkyboxShader.metal
//  ios
//
//  Created by Nicolas Märki on 18.05.23.
//

#include <metal_stdlib>
#include "DataStructures.metal"
using namespace metal;



float hash(float n)
{
  return fract(sin(n) * 43758.5453123);
}

float noise(float3 x)
{
  float3 f = fract(x);
  float n = dot(floor(x), float3(1.0, 157.0, 113.0));
  return mix(mix(mix(hash(n +   0.0), hash(n +   1.0), f.x),
                 mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
             mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
                 mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

float fbm(float3 p)
{
  float3x3 m = float3x3(0.0, 1.60,  1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
  float f = 0.0;
  f += noise(p) / 2; p = m * p * 1.1;
  f += noise(p) / 4; p = m * p * 1.2;
  f += noise(p) / 6; p = m * p * 1.3;
  f += noise(p) / 12; p = m * p * 1.4;
  f += noise(p) / 24;
  return f;
}

fragment float4
skyboxFragmentShader(const VertexOut vertexIn [[stage_in]],
                     constant float4x4 &matrix [[buffer(0)]],
                     constant float &time [[buffer(1)]])
{


  float3 fsun = normalize(float3(sin(time*0.1), cos(time*0.1), 1.0));

  // Compute the normalized direction vector from the texture coordinates
  float verticalFactor = vertexIn.uv.y;
  float3 pos = float3(vertexIn.uv.x * 2.0 - 1.0, verticalFactor, sqrt(1.0 - verticalFactor * verticalFactor));
  pos = normalize(pos);

  float cirrus = 0.7;
  float cumulus = 0.1;

  const float Br = 0.0025;
  const float Bm = 0.0003;
  const float g =  0.9800;
  const float3 nitrogen = float3(0.650, 0.570, 0.475);
  const float3 Kr = Br / pow(nitrogen, float3(4.0));
  const float3 Km = Bm / pow(nitrogen, float3(0.84));

  // Atmosphere Scattering
  float mu = dot(normalize(pos), normalize(fsun));
  float rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
  float3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

  float3 color;

  float3 day_extinction = exp(-exp(-((pos.y + fsun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0 ) * 4.0) * exp(-pos.y * 2.0) * 4.0;
  float3 night_extinction = float3(1.0 - exp(fsun.y)) * 0.2;
  float3 extinction = mix(day_extinction, night_extinction, -fsun.y * 0.2 + 0.5);
  color.rgb = rayleigh * mie * extinction;

  // Cirrus Clouds
  float density = smoothstep(1.0 - cirrus, 1.0, fbm(pos.xyz / pos.y * 2.0 + time * 0.05)) * 0.3;
  color.rgb = mix(color.rgb, extinction * 4.0, density * max(pos.y, 0.0));

  // Cumulus Clouds
  for (int i = 0; i < 10; i++)
  {
    float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7 + float(i) * 0.01) * pos.xyz / pos.y + time * 0.3));
    color.rgb = mix(color.rgb, extinction * density * 5.0, min(density, 1.0) * max(pos.y, 0.0));
  }

  // Dithering Noise
  color.rgb += noise(pos * 1000) * 0.01;

  return float4(color, 1.0);
}

