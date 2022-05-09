#include "Common.hlsl"

RWTexture2D<float4> transmittanceMap : register(u0);

float DensityOverPath(in float scaleHeight, in float alt, in float mu)
{
    // if ray below horizon return max density
    float cosHorizon = -sqrt(1.0f - ((Rg * Rg) / (alt * alt)));
    if (mu < cosHorizon)
        return 1e9;

    float totalDensity = 0.0f;
    float dx = IntersectAtmosphere(alt, mu) / float(TRANSMITTANCE_INTEGRAL_SAMPLES);

    float y_j = exp(-(alt - Rg) / scaleHeight);

    for (int i = 1; i <= TRANSMITTANCE_INTEGRAL_SAMPLES; ++i)
    {
        float x_i = float(i) * dx;
        float alt_i = sqrt(alt * alt + x_i * x_i + 2.0f * x_i * alt * mu);
        float y_i = exp(-(alt_i - Rg) / scaleHeight);
        totalDensity += (y_j + y_i) / 2.0f * dx;
        y_j = y_i;
    }
    return totalDensity;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    uint2 dims;
    transmittanceMap.GetDimensions(dims.x, dims.y);
    
    float u = groupId.x / float(dims.x);
    float v = groupId.y / float(dims.y);
    
    float2 altMu = GetTransmittanceAltMu(u, v);
    float3 t = betaR * DensityOverPath(HR, altMu.x, altMu.y)
             + betaM * DensityOverPath(HM, altMu.x, altMu.y);
    
    float3 transmittance = exp(-t);
    transmittanceMap[groupId.xy] = float4(transmittance, 0.0f);
}
