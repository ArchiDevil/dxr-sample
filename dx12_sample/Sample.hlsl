#include "Common.h"

struct RayPayload
{
    float4 color;
};

RWTexture2D<float4> output : register(u0); // render target
RaytracingAccelerationStructure scene : register(t0, space0);
ConstantBuffer<ViewParams> sceneParams : register(b0);
ConstantBuffer<LightParams> lightParams : register(b1);

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

[shader("closesthit")]
void ClosestHitShader_Triangle(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    float4 barycentrics = float4(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y, 1.0);
    rayPayload.color    = float4(0.5, 0.5, 0.5, 1.0);
}
