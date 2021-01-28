#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
    float2 n [[attribute(2)]];
};

struct VertexOut {
    float4 position [[ position ]];
    float2 uv;
    float pointsize [[ point_size ]];
};

