#include "Common.h"

struct RayPayload
{
    float4 color;
};

RWTexture2D<float4>              output       : register(u0); // render target

ConstantBuffer<ViewParams>       sceneParams  : register(b0);
ConstantBuffer<LightParams>      lightParams  : register(b1);
ConstantBuffer<ModelParams>      modelParams  : register(b2);

RaytracingAccelerationStructure  scene        : register(t0, space0);
StructuredBuffer<GeometryVertex> vertexBuffer : register(t1);
ByteAddressBuffer                indexBuffer  : register(t2);

inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0; // [-1.0; 1.0]

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), sceneParams.inverseViewProj);
    world.xyz /= world.w;

    origin    = sceneParams.viewPos.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void RayGenShader()
{
    RayPayload payload;
    payload.color = float4(0.0, 1.0, 0.0, 1.0);
    
    uint3 launchIndex = DispatchRaysIndex();
    RayDesc desc;
    desc.TMin = 0.01f;
    desc.TMax = 1000.0f;
    GenerateCameraRay(launchIndex.xy, desc.Origin, desc.Direction);

    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, desc, payload);

    output[launchIndex.xy] = payload.color;
}

[shader("miss")]
void MissShader(inout RayPayload ray)
{
    ray.color = float4(0.6, 0.75, 1.0, 1.0);
}

float4 DirectLight(float4 normal) {
    return saturate(dot(float4(normal), lightParams.direction)) * lightParams.color;
}

uint3 LoadIndices(uint primitiveIndex)
{
    uint formatSize = 4; // 4 bytes per index
    uint stride = formatSize * 3; // 3 indices per triangle
    uint shift = stride * primitiveIndex;

    uint3 output = uint3(0, 0, 0);
    for (int i = 0; i < 3; ++i)
    {
        uint indexBytes = indexBuffer.Load(shift + i);
        output[i] = indexBytes;
    }

    return output;
}

float2 Interpolate(float2 attribute1, float2 attribute2, float2 attribute3, float3 barycentrics)
{
    return attribute1 * barycentrics.x + attribute2 * barycentrics.y + attribute3 * barycentrics.z;
}

float3 Interpolate(float3 attribute1, float3 attribute2, float3 attribute3, float3 barycentrics)
{
    return attribute1 * barycentrics.x + attribute2 * barycentrics.y + attribute3 * barycentrics.z;
}

GeometryVertex LoadAndInterpolate(uint3 indices, float3 barycentrics)
{
    GeometryVertex v1, v2, v3;
    v1 = vertexBuffer.Load(indices.x);
    v2 = vertexBuffer.Load(indices.y);
    v3 = vertexBuffer.Load(indices.z);
    
    GeometryVertex output;
    output.position = Interpolate(v1.position, v2.position, v3.position, barycentrics);
    output.normal = Interpolate(v1.normal, v2.normal, v3.normal, barycentrics);
    output.binormal = Interpolate(v1.binormal, v2.binormal, v3.binormal, barycentrics);
    output.tangent = Interpolate(v1.tangent, v2.tangent, v3.tangent, barycentrics);
    output.uv = Interpolate(v1.uv, v2.uv, v3.uv, barycentrics);
    return output;
}

[shader("closesthit")]
void ClosestHitShader_Triangle(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    // Load vertices
    GeometryVertex v = LoadAndInterpolate(LoadIndices(PrimitiveIndex()), barycentrics);

    float3 n = normalize(mul(float4(v.normal, 0.0), transpose(modelParams.worldMatrix))).xyz;
    float3 l = normalize(lightParams.direction.xyz);

    float lightness = saturate(dot(n, l));

    float3 diffuseColor = modelParams.color.xyz * lightness * lightParams.color.xyz;
    float3 ambientColor = sceneParams.ambientColor.xyz;
    rayPayload.color = float4(diffuseColor + ambientColor, 1.0);
}
