#include "Common.h"

struct RayPayload
{
    float4 color;
};

RWTexture2D<float4> output : register(u0); // render target
RaytracingAccelerationStructure scene : register(t0, space0);
ConstantBuffer<ViewParams> sceneParams : register(b0);

inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0; // [-1.0; 1.0]

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(float4(screenPos, 0, 1), sceneParams.inverseViewProj);

    world.xyz /= world.w;
    origin = sceneParams.viewPos.xyz;
    direction = normalize(world.xyz - origin);
}

[shader("raygeneration")]
void RayGenShader()
{
    RayPayload payload;
    payload.color = float4(0.0, 1.0, 0.0, 1.0);
    
    uint3 launchIndex = DispatchRaysIndex();
    RayDesc desc;
    GenerateCameraRay(launchIndex.xy, desc.Origin, desc.Direction);

    TraceRay(scene, 0, 0, 0, 0, 0, desc, payload);

    output[launchIndex.xy] = payload.color;
}

[shader("miss")]
void MissShader(inout RayPayload ray)
{
    ray.color = float4(0.4, 0.4, 0.9, 1.0);
}
