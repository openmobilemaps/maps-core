//
//  SkyboxShader.metal
//  ios
//
//  Created by Nicolas Märki on 18.05.23.
//

#include <metal_stdlib>
#include "DataStructures.metal"
using namespace metal;



// Helper function to compute the Perez function
float perezFunction(float cosTheta, float cosGamma, float a, float b, float c, float d, float e) {
  return (1.0 + a * exp(b / cosTheta)) * (1.0 + c * exp(d * cosGamma) + e * pow(cosGamma, 2));
}

fragment float4
skyboxFragmentShader(const VertexOut vertexIn [[stage_in]])
{

  // Constants
  float3 zenithColor = float3(0.25, 0.25, 0.55);
  float3 horizonColor = float3(0.65, 0.4, 0.3);
  float3 sunDirection = normalize(float3(0.0, 0.0, -1.0));
  float turbidity = 0.0;

  float3 color;
  float verticalFactor = vertexIn.uv.y;

  // Compute the normalized direction vector from the texture coordinates
  float3 direction = float3(vertexIn.uv.x * 2.0 - 1.0, verticalFactor, sqrt(1.0 - verticalFactor * verticalFactor));
  direction = normalize(direction);

  // Compute the sun zenith angle and the angle between the sun and the direction
  float cosTheta = clamp(direction.y, -1.0, 1.0);
  float cosGamma = clamp(dot(direction, sunDirection), 0.0, 1.0);

  // Compute the coefficients for the Perez function
  float a = (0.1787 * turbidity - 1.4630) / 0.459;
  float b = (-0.0193 * turbidity + 0.1595) / 0.078;
  float c = (-0.0167 * turbidity + 0.4275) / 0.459;
  float d = (-0.1049 * turbidity + 0.6965) / 0.078;
  float e = (-0.0109 * turbidity + 0.1117) / 0.5;

  float p = perezFunction(cosTheta, cosGamma, a, b, c, d, e);
  p = verticalFactor * 0.5+0.5;

  // Compute the final color
  color = mix(horizonColor, zenithColor,  p);

  // Output color
  return float4(color, 1.0);
//  return float4(cosTheta, cosGamma, 0.0, 1.0);
  return float4(p, p*0.1, p*0.01, 1.0);
}

