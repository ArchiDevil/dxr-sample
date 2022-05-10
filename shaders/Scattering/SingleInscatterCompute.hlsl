#include "Common.hlsl"

RWTexture3D<float4> deltaSR          : register(u0); // rayleigh delta table
RWTexture3D<float4> deltaSM          : register(u1); // mie delta table
RWTexture3D<float4> inscatter        : register(u2); // final table
RWTexture2D<float4> transmittanceMap : register(u3); // precomputed transmittance map (READ)

void Integrate(in float alt, in float mu, in float mus, in float nu,
               in float dist, out float3 ray, out float3 mie)
{
    ray = float3(0.0, 0.0, 0.0);
    mie = float3(0.0, 0.0, 0.0);

    float alt_i = sqrt(alt * alt + dist * dist + 2.0 * alt * mu * dist);
    float mus_i = (nu * dist + mus * alt) / alt_i;

    alt_i = max(Rg, alt_i);

    // if angle between zenith and sun smaller than angle to horizon
    // return ray and mie values
    if (mus_i >= -sqrt(1.0 - ((Rg * Rg) / (alt_i * alt_i))))
    {
        float3 trans = Transmittance(alt, mu, dist, transmittanceMap) * Transmittance(alt_i, mus_i, transmittanceMap);
        ray = exp(-(alt_i - Rg) / HR) * trans;
        mie = exp(-(alt_i - Rg) / HM) * trans;
    }
}

void Inscatter(float alt, float mu, float mus, float nu,
               out float3 ray, out float3 mie)
{
    ray = float3(0.0, 0.0, 0.0);
    mie = float3(0.0, 0.0, 0.0);

    float dx = IntersectAtmosphere(alt, mu) / float(INSCATTER_INTEGRAL_SAMPLES);

    float x_i = 0.0;
    float3 ray_i, mie_i;

    Integrate(alt, mu, mus, nu, 0.0, ray_i, mie_i);

    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i)
    {
        float x_j = float(i) * dx;
        float3 ray_j;
        float3 mie_j;
        Integrate(alt, mu, mus, nu, x_j, ray_j, mie_j);
        ray += (ray_i + ray_j) / 2.0 * dx;
        mie += (mie_i + mie_j) / 2.0 * dx;
        x_i = x_j;

        ray_i = ray_j;
        mie_i = mie_j;
    }
    
    ray *= betaR;
    mie *= betaMSca;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    uint3 dims;
    deltaSR.GetDimensions(dims.x, dims.y, dims.z); // all outputs have the same dimensions

    float u = (float) groupId.x / dims.x;
    float v = (float) groupId.y / dims.y;

    uint tableWidth = dims.x;
    uint tableHeight = dims.y;
    uint resNu = RES_NU;

    uint layer = groupId.z;
    uint altLayers = dims.z + 1;

    float alt = layer / (altLayers - 1.0);
    alt = alt * alt;
    alt = sqrt(Rg * Rg + alt * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == altLayers - 1 ? -0.001 : 0.0));
    float4 dhdH = GetDhdH(alt);

    float3 ray, mie;
    float3 muMusNu = GetMuMusNu(alt, dhdH, u, v, tableWidth, tableHeight, resNu);

    Inscatter(alt, muMusNu.x, muMusNu.y, muMusNu.z, ray, mie);

    deltaSR[groupId] = float4(ray, 1.0);
    deltaSM[groupId] = float4(mie, 1.0);
    
    // we store a first iteration of the algorithm here, other will be calculated and added later
    inscatter[groupId] = float4(ray, mie.r);
}
