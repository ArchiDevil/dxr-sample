#include "Common.hlsl"

RWTexture3D<float4> inscatterMap : register(u0);
RWTexture3D<float4> deltaSR      : register(u1);

[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    const float resNu = RES_NU;
    uint3 dims;
    inscatterMap.GetDimensions(dims.x, dims.y, dims.z);

    float alt = GetAlt(groupId.z, dims.z);
    float4 dhdH = GetDhdH(alt);
    float3 muMusNu = GetMuMusNu(alt, dhdH, float(groupId.x), float(groupId.y), dims.x, dims.y, resNu);
    float3 sample = SampleTexture4D(deltaSR, alt, muMusNu.x, muMusNu.y, muMusNu.z, dims.x, resNu).rgb / PhaseFunctionR(muMusNu.z);
    inscatterMap[groupId] += float4(sample, 0.0);
}
