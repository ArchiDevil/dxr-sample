#include "Common.hlsl"

RWTexture2D<float4> deltaE           : register(u0);
RWTexture2D<float4> transmittanceMap : register(u1);

// This shader calculates a single irradiance that is needed later to calculate multisampled irradiance map
// The intermediate map for that is called deltaE
[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    uint2 dims;
    deltaE.GetDimensions(dims.x, dims.y);

    float u = groupId.x / float(dims.x);
    float v = groupId.y / float(dims.y);

    float2 altMus = GetIrradianceAltMus(u, v);

    float3 attenuation = Transmittance(altMus.x, altMus.y, transmittanceMap);
    deltaE[groupId.xy] = float4(attenuation * saturate(altMus.y), 0.0);
}
