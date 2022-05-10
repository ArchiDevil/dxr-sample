#include "Common.hlsl"

RWTexture3D<float4> inscatterMap : register(u0);
RWTexture3D<float4> deltaSR      : register(u1);

[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    const float resNu = RES_NU;
    uint3 dims;
    inscatterMap.GetDimensions(dims.x, dims.y, dims.z);
    
    float u = (float) groupId.x / dims.x;
    float v = (float) groupId.y / dims.y;
    
    uint layer = groupId.z;
    uint altLayers = dims.z + 1;

    float alt = layer / (altLayers - 1.0);
    alt = alt * alt;
    alt = sqrt(Rg * Rg + alt * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == altLayers - 1 ? -0.001 : 0.0));
    
    float4 dhdH = GetDhdH(alt);
    float3 muMusNu = GetMuMusNu(alt, dhdH, u, v, dims.x, dims.y, resNu);
    float3 sample = SampleTexture4D(deltaSR, alt, muMusNu.x, muMusNu.y, muMusNu.z, dims.x, resNu).rgb / PhaseFunctionR(muMusNu.z);
    inscatterMap[groupId] += float4(sample, 0.0);
}
