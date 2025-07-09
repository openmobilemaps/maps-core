//
//  QuadTessellation.metal
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

#include <metal_stdlib>
#include "DataStructures.metal"

// A struct to hold our control point data
struct ControlPoint {
    float3 position;
    float2 textureCoordinate;
};

// Post-tessellation vertex function for 2D quads
// --- API CHANGE ---
// The number of control points must be 4 for a quad patch.
[[patch(quad, 4)]]
vertex VertexOut tessellatedQuadVertex(
    constant ControlPoint *controlPoints [[buffer(0)]],
    constant float4x4 &vpMatrix [[buffer(1)]],
    constant float4x4 &mMatrix [[buffer(2)]],
    constant float4 &originOffset [[buffer(3)]],
    float2 positionInPatch [[position_in_patch]] // The (u,v) coordinate from the tessellator
) {
    // controlPoints order: Bottom-Left, Top-Left, Top-Right, Bottom-Right

    // Bilinear interpolation for position
    float3 bottom = mix(controlPoints[0].position, controlPoints[3].position, positionInPatch.x);
    float3 top = mix(controlPoints[1].position, controlPoints[2].position, positionInPatch.x);
    float3 p = mix(top, bottom, positionInPatch.y);

    // Bilinear interpolation for texture coordinates
    float2 texBottom = mix(controlPoints[0].textureCoordinate, controlPoints[3].textureCoordinate, positionInPatch.x);
    float2 texTop = mix(controlPoints[1].textureCoordinate, controlPoints[2].textureCoordinate, positionInPatch.x);
    float2 uv = mix(texTop, texBottom, positionInPatch.y);

    VertexOut out;
    out.position = vpMatrix * ((mMatrix * float4(p, 1.0)) + originOffset);
    out.uv = uv;

    return out;
}

// Post-tessellation vertex function for 3D spherical quads
// --- API CHANGE ---
// The number of control points must be 4 for a quad patch.
[[patch(quad, 4)]]
vertex VertexOut tessellatedQuadVertex3D(
    constant ControlPoint *controlPoints [[buffer(0)]],
    constant float4x4 &vpMatrix [[buffer(1)]],
    constant float4x4 &mMatrix [[buffer(2)]],
    constant float4 &originOffset [[buffer(3)]],
    constant float4 &origin [[buffer(4)]],
    float2 positionInPatch [[position_in_patch]]
) {
    // This logic replicates the bilinear interpolation of lat/lon coordinates from the original Swift code.
    float3 topLeft = controlPoints[1].position;
    float3 topRight = controlPoints[2].position;
    float3 bottomLeft = controlPoints[0].position;
    float3 bottomRight = controlPoints[3].position;

    float pcR = positionInPatch.x;
    float pcD = positionInPatch.y;

    float3 deltaRTop = topRight - topLeft;
    float3 deltaDLeft = bottomLeft - topLeft;
    float3 deltaDRight = bottomRight - topRight;

    float3 interpolatedLatLon = topLeft + pcR * deltaRTop + pcD * mix(deltaDLeft, deltaDRight, pcR);

    // Transform from lat/lon (stored in finalPos) to 3D Cartesian on the sphere
    float x = 1.0 * sin(interpolatedLatLon.y) * cos(interpolatedLatLon.x) - origin.x;
    float y = 1.0 * cos(interpolatedLatLon.y) - origin.y;
    float z = -1.0 * sin(interpolatedLatLon.y) * sin(interpolatedLatLon.x) - origin.z;
    float3 transformedPos = float3(x, y, z);

    // Bilinear interpolation for texture coordinates
    float2 texBottom = mix(controlPoints[0].textureCoordinate, controlPoints[3].textureCoordinate, pcR);
    float2 texTop = mix(controlPoints[1].textureCoordinate, controlPoints[2].textureCoordinate, pcR);
    float2 uv = mix(texTop, texBottom, pcD);

    VertexOut out;
    out.position = vpMatrix * ((mMatrix * float4(transformedPos, 1.0)) + originOffset);
    out.uv = uv;

    return out;
}
