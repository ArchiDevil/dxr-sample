#include "Common.hlsl"

RWTexture3D<float4> irradianceMap : register(u0);
Texture2D transmittanceMap        : register(t0);

float3 Transmittance(float alt, float mu)
{
    alt -= Rg;
    alt /= Rt - Rg;

    uint2 dims;
    transmittanceMap.GetDimensions(dims.x, dims.y);
    uint2 coords = uint2((uint)(dims.x * mu), (uint)(dims.y * alt));
    return transmittanceMap[coords].rgb;
}

float3 Transmittance(float alt, float mu, float dist)
{
    float alt_i = sqrt(alt * alt + dist * dist + 2.0f * dist * alt * mu);
    return Transmittance(alt_i, mu).rgb;
}

void Integrate(in float alt, in float mu, in float mus, in float nu,
               in float dist, out float3 ray, out float3 mie)
{
    ray = float3(0.0f, 0.0f, 0.0f);
    mie = float3(0.0f, 0.0f, 0.0f);

    float alt_i = sqrt(alt * alt + dist * dist + 2.0 * alt * mu * dist);
    float mus_i = (nu * dist + mus * alt) / alt_i;

    alt_i = max(Rg, alt_i);

    // if angle between zenith and sun smaller than angle to horizon
    // return ray and mie values
    if (mus_i >= -sqrt(1.0f - ((Rg * Rg) / (alt_i * alt_i))))
    {
        float3 trans = Transmittance(alt, mu, dist) * Transmittance(alt_i, mus_i);
        ray = exp(-(alt_i - Rg) / HR) * trans;
        mie = exp(-(alt_i - Rg) / HM) * trans;
    }
}

void Inscatter(float alt, float mu, float mus, float nu,
               out float3 ray, out float3 mie)
{
    ray = float3(0.0f, 0.0f, 0.0f);
    mie = float3(0.0f, 0.0f, 0.0f);

    float dx = IntersectAtmosphere(alt, mu) / float(INSCATTER_INTEGRAL_SAMPLES);

    float x_i = 0.0;
    float3 ray_i, mie_i;

    Integrate(alt, mu, mus, nu, 0.0f, ray_i, mie_i);

    for (int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i)
    {
        float x_j = float(i) * dx;
        float3 ray_j;
        float3 mie_j;
        Integrate(alt, mu, mus, nu, x_j, ray_j, mie_j);
        ray += (ray_i + ray_j) / 2.0f * dx;
        mie += (mie_i + mie_j) / 2.0f * dx;
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
    irradianceMap.GetDimensions(dims.x, dims.y, dims.z);

    float u = (float)groupId.x / dims.x;
    float v = (float)groupId.y / dims.y;
    float w = (float)groupId.z / dims.z;

    float nu = cos(PI * (1 - u) /* * (u / 8)*/); // view-sun angle
    float mus = cos(PI_2 * (1 - u) /* * (u % 8)*/); // sun-zenith angle

    float mu = cos(PI_2 * (1 - v)); // view-zenith angle
    float alt = w * (Rt - Rg) + Rg;  // altitude

    float3 ray, mie;
    Inscatter(alt, mu, mus, nu, ray, mie);

    irradianceMap[groupId] = float4(ray, mie.r);
}
