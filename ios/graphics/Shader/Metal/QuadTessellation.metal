//
//  QuadTessellation.metal
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

#include <metal_stdlib>
#include "DataStructures.metal"

using namespace metal;

struct ControlPoint {
    float3 position;
    float2 textureCoordinate;
    float3 normal;
};


[[patch(quad, 4)]]
vertex VertexOut tessellatedQuadVertex(
    constant ControlPoint *controlPoints [[buffer(0)]],
    constant float4x4 &vpMatrix [[buffer(1)]],
    constant float4x4 &mMatrix [[buffer(2)]],
    constant float4 &originOffset [[buffer(3)]],
    float2 positionInPatch [[position_in_patch]]
) {
    float u = positionInPatch.x;
    float v = positionInPatch.y;

    // Bilinear interpolation for the pre-calculated positions.
    // These positions are already relative to the object's origin.
    float3 bottom = mix(controlPoints[0].position, controlPoints[3].position, u);
    float3 top = mix(controlPoints[1].position, controlPoints[2].position, u);
    float3 p = mix(top, bottom, v);

    // Bilinear interpolation for texture coordinates.
    float2 texBottom = mix(controlPoints[0].textureCoordinate, controlPoints[3].textureCoordinate, u);
    float2 texTop = mix(controlPoints[1].textureCoordinate, controlPoints[2].textureCoordinate, u);
    float2 uv = mix(texTop, texBottom, v);

    VertexOut out;
    // The final vertex position is calculated exactly like the original base shader.
    // 'p' is the local model-space position. We add the originOffset to get the world-space position.
    out.position = vpMatrix * ((mMatrix * float4(p, 1.0)) + originOffset);
    out.uv = uv;

    return out;
}


[[patch(quad, 4)]]
vertex VertexOut tessellatedQuadVertex3D(
    constant ControlPoint *controlPoints [[buffer(0)]],
    constant float4x4 &vpMatrix [[buffer(1)]],
    constant float4x4 &mMatrix [[buffer(2)]],
    constant float4 &originOffset [[buffer(3)]],
    constant float4 &origin [[buffer(4)]],       // The absolute coordinate of the object's origin
    float2 positionInPatch [[position_in_patch]]
) {
    float u = positionInPatch.x;
    float v = positionInPatch.y;

    // --- Step 1: Calculate the starting point on the flat quad ---
    // We interpolate the high-precision relative positions. This is our trusted starting point.
    float3 posBottom = mix(controlPoints[0].position, controlPoints[3].position, u);
    float3 posTop = mix(controlPoints[1].position, controlPoints[2].position, u);
    float3 interpolatedPosition = mix(posTop, posBottom, v);

    // --- Step 2: Calculate the target destination on the sphere ---
    // We interpolate the absolute target positions. This tells us where we need to go.
    float3 targetBottom = mix(controlPoints[0].normal, controlPoints[3].normal, u);
    float3 targetTop = mix(controlPoints[1].normal, controlPoints[2].normal, u);
    float3 interpolatedAbsoluteTarget = normalize(mix(targetTop, targetBottom, v));

    // --- Step 3: Calculate the displacement vector ---
    // To find the vector between our start and end points, they must be in the same coordinate system.
    // Let's convert our relative starting point to an absolute one.
    float3 initialAbsolutePosition = interpolatedPosition + origin.xyz;

    // The displacement vector is the difference between the target and the start point.
    float3 displacementVector = interpolatedAbsoluteTarget - initialAbsolutePosition;

    // --- Step 4: Calculate the final relative position ---
    // Apply the calculated displacement to our original high-precision starting point.
    // The result `p` is the final, correct position, still relative to the object's origin.
    float3 p = interpolatedPosition + displacementVector;

    // --- Step 5: Interpolate Texture Coordinates ---
    float2 texBottom = mix(controlPoints[0].textureCoordinate, controlPoints[3].textureCoordinate, u);
    float2 texTop = mix(controlPoints[1].textureCoordinate, controlPoints[2].textureCoordinate, u);
    float2 uv = mix(texTop, texBottom, v);

    VertexOut out;

    // --- Step 6: Final Transformation ---
    // The rest of the pipeline works perfectly because `p` is the correct, relative position.
    out.position = vpMatrix * ((mMatrix * float4(p, 1.0)) + originOffset);
    out.uv = uv;

    return out;
}
