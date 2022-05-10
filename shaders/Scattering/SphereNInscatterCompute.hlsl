#include "Common.hlsl"

RWTexture3D<float4> deltaSR          : register(u0);
RWTexture3D<float4> deltaJ           : register(u1);
RWTexture2D<float4> transmittanceMap : register(u2);

float3 Integrate(float alt, float mu, float mus, float nu, float dist, uint tableWidth, uint resNu)
{
    float alti = sqrt(alt * alt + dist * dist + 2.0 * alt * mu * dist);
    float mui = (alt * mu + dist) / alti;
    float muSi = (nu * dist + mus * alt) / alti;
    return SampleTexture4D(deltaJ, alti, mui, muSi, nu, tableWidth, resNu).rgb * Transmittance(alt, mu, dist, transmittanceMap);
}

float3 Inscatter(float alt, float mu, float mus, float nu, uint tableWidth, uint resNu)
{
    float3 raymie = float3(0, 0, 0);
    float dx = Limit(alt, mu) / float(INSCATTER_INTEGRAL_SAMPLES);
    float xi = 0.0;
    float3 raymiei = Integrate(alt, mu, mus, nu, 0.0, tableWidth, resNu);
    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i)
    {
        float xj = float(i) * dx;
        float3 raymiej = Integrate(alt, mu, mus, nu, xj, tableWidth, resNu);
        raymie += (raymiei + raymiej) / 2.0 * dx;
        xi = xj;
        raymiei = raymiej;
    }
    return raymie;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    const float resNu = RES_NU;

    uint3 dims;
    deltaSR.GetDimensions(dims.x, dims.y, dims.z);

    float alt = GetAlt(groupId.z, dims.z);
    float4 dhdH = GetDhdH(alt);
    float3 muMusNu = GetMuMusNu(alt, dhdH, float(groupId.x), float(groupId.y), dims.x, dims.y, resNu);
    float3 result = Inscatter(alt, muMusNu.x, muMusNu.y, muMusNu.z, dims.y, resNu);
    
    deltaSR[groupId] = float4(result, 0.0);
}
