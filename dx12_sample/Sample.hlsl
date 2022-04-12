struct RayPayload
{
    float4 color;
};

RWTexture2D<float4> output : register(u0); // render target
RaytracingAccelerationStructure scene : register(t0, space0);

[shader("raygeneration")]
void RayGenShader()
{
    RayPayload payload;
    payload.color = float4(0.0, 1.0, 0.0, 1.0);
    
    uint3 launchIndex = DispatchRaysIndex();
    RayDesc desc;
    desc.Origin = float3(0.0, 0.0, 0.0);
    desc.Direction = float3(1.0, 0.0, 0.0);

    TraceRay(scene, 0, 0, 0, 0, 0, desc, payload);

    output[launchIndex.xy] = payload.color;
}

[shader("miss")]
void MissShader(inout RayPayload ray)
{
    ray.color = float4(0.4, 0.4, 0.9, 1.0);
}
