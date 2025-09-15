#include <metal_stdlib>
#include "DataStructures.metal"
using namespace metal;

// Decodes elevation in meters from an RGBA-encoded pixel.
// Matches the provided formula: ((r*256*256*255) + (g*256*255) + (b*255))/10 - 10000
inline float decodeElevation(half4 rgbaAltitude) {
    return (rgbaAltitude.r * 256.0 * 256.0 * 255.0 +
            rgbaAltitude.g * 256.0 * 255.0 +
            rgbaAltitude.b * 255.0) / 10.0 - 10000.0;
}

// Encodes elevation in meters back into RGB using the inverse of the above scheme.
// We map elevation to a 24-bit integer value V in range [0, 256*256*256-1] where
// elevationMeters = V/10 - 10000  =>  V = (elevationMeters + 10000) * 10
inline half4 encodeElevation(float elevationMeters) {
    // Clamp to representable range [0, 16777215]
    float value = (elevationMeters + 10000.0) * 10.0;
    value = clamp(value, 0.0, 16777215.0);

    // Convert to 24-bit integer and split across R,G,B components.
    uint V = (uint)round(value);
    uint r = (V / (256u * 256u)) & 255u;
    uint g = (V / 256u) & 255u;
    uint b = V & 255u;

    // Normalize to [0,1] range for storage in 8-bit per channel texture.
    half4 outColor = half4((float)r / 255.0, (float)g / 255.0, (float)b / 255.0, 1.0);
    return outColor;
}

// Simple bilinear sampling helper that decodes elevation from four neighbors and blends.
inline float bilinearSampleElevation(texture2d<half> srcTex,
                                     sampler s,
                                     float2 uv) {
    // Sample the four neighboring texels around the target coordinate.
    float2 texSize = float2(srcTex.get_width(), srcTex.get_height());
    float2 coord = uv * texSize - 0.5; // shift so integer coords land at texel centers

    float2 base = floor(coord);
    float2 f = coord - base; // fractional part for weights

    float2 uv00 = (base + float2(0.0, 0.0) + 0.5) / texSize;
    float2 uv10 = (base + float2(1.0, 0.0) + 0.5) / texSize;
    float2 uv01 = (base + float2(0.0, 1.0) + 0.5) / texSize;
    float2 uv11 = (base + float2(1.0, 1.0) + 0.5) / texSize;

    half4 c00 = srcTex.sample(s, uv00);
    half4 c10 = srcTex.sample(s, uv10);
    half4 c01 = srcTex.sample(s, uv01);
    half4 c11 = srcTex.sample(s, uv11);

    float e00 = decodeElevation(c00);
    float e10 = decodeElevation(c10);
    float e01 = decodeElevation(c01);
    float e11 = decodeElevation(c11);

    // Bilinear interpolation
    float e0 = mix(e00, e10, f.x);
    float e1 = mix(e01, e11, f.x);
    float e = mix(e0, e1, f.y);

    return e;
}

fragment half4
elevationInterpolationFragmentShader(VertexOut in [[stage_in]],
                   constant float &alpha [[buffer(1)]],
                   texture2d<half> texture0 [[ texture(0)]],
                   sampler textureSampler [[sampler(0)]])
{
    float elevation = bilinearSampleElevation(texture0, textureSampler, in.uv);
    return encodeElevation(elevation);
}
