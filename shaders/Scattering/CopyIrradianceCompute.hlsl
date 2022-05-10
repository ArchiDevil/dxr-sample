RWTexture2D<float4> irradianceMap : register(u0);
RWTexture2D<float4> deltaE        : register(u1);

[numthreads(1, 1, 1)]
void CSMain(uint2 groupId : SV_GroupID)
{
    float3 irradianceData = irradianceMap[groupId].rgb;
    float3 deltaEData = deltaE[groupId].rgb;
    irradianceMap[groupId] = float4(irradianceData + deltaEData, 0.0);
}
