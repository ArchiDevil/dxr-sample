#include "Common.hlsl"

RWTexture3D<float4> deltaSM       : register(u0);
RWTexture3D<float4> deltaSR       : register(u1);
RWTexture3D<float4> deltaJ        : register(u2);
RWTexture2D<float4> transmittance : register(u3);
RWTexture2D<float4> deltaE        : register(u4);

uint first : register(c0);

float3 Inscatter(float alt, float mu, float mus, float nu, uint tableWidth, uint resNu)
{
    const float dphi = PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);
    const float dtheta = PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);

    alt = clamp(alt, Rg, Rt);
    mu = clamp(mu, -1.0, 1.0);
    mus = clamp(mus, -1.0, 1.0);
    float var = sqrt(1.0 - mu * mu) * sqrt(1.0 - mus * mus);
    nu = clamp(nu, mus * mu - var, mus * mu + var);

    float cthetamin = -sqrt(1.0 - (Rg / alt) * (Rg / alt));

    float3 v = float3(sqrt(1.0 - mu * mu), 0.0, mu);
    float sx = v.x == 0.0 ? 0.0 : (nu - mus * mu) / v.x;
    float3 s = float3(sx, sqrt(max(0.0, 1.0 - sx * sx - mus * mus)), mus);

    float3 raymie = float3(0, 0, 0);

    // integral over 4.PI around x with two nested loops over w directions (theta,phi) -- Eq (7)
    for (int itheta = 0; itheta < INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++itheta)
    {
        float theta = (float(itheta) + 0.5) * dtheta;
        float ctheta = cos(theta);

        float greflectance = 0.0;
        float dground = 0.0;
        float3 gtransp = float3(0, 0, 0);
        if (ctheta < cthetamin)
        {
            // if ground visible in direction w
            // compute transparency gtransp between x and ground
            greflectance = AVERAGE_GROUND_REFLECTANCE / PI;
            dground = -alt * ctheta - sqrt(alt * alt * (ctheta * ctheta - 1.0) + Rg * Rg);
            gtransp = Transmittance(Rg, -(alt * ctheta + dground) / Rg, dground, transmittance);
        }

        for (int iphi = 0; iphi < 2 * INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++iphi)
        {
            float phi = (float(iphi) + 0.5) * dphi;
            float dw = dtheta * dphi * sin(theta);
            float3 w = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), ctheta);

            float nu1 = dot(s, w);
            float nu2 = dot(v, w);
            float pr2 = PhaseFunctionR(nu2);
            float pm2 = PhaseFunctionM(nu2);

            // compute irradiance received at ground in direction w (if ground visible) =deltaE
            float3 gnormal = (float3(0.0, 0.0, alt) + dground * w) / Rg;
            float3 girradiance = Irradiance(deltaE, Rg, dot(gnormal, s));

            float3 raymie1; // light arriving at x from direction w

            // first term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE
            raymie1 = greflectance * girradiance * gtransp;

            // second term = inscattered light, =deltaS
            if (first == 1)
            {
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = PhaseFunctionR(nu1);
                float pm1 = PhaseFunctionM(nu1);
                float3 ray1 = SampleTexture4D(deltaSR, alt, w.z, mus, nu1, tableWidth, resNu).rgb;
                float3 mie1 = SampleTexture4D(deltaSM, alt, w.z, mus, nu1, tableWidth, resNu).rgb;
                raymie1 += ray1 * pr1 + mie1 * pm1;
            }
            else
            {
                raymie1 += SampleTexture4D(deltaSR, alt, w.z, mus, nu1, tableWidth, resNu).rgb;
            }

            // light coming from direction w and scattered in direction v
            // = light arriving at x from direction w (raymie1) * SUM(scattering coefficient * phaseFunction)
            // see Eq (7)
            raymie += raymie1 * (betaR * exp(-(alt - Rg) / HR) * pr2 + betaMSca * exp(-(alt - Rg) / HM) * pm2) * dw;
        }
    }
    
    return raymie;

    // output raymie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1)
}

[numthreads(1, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID)
{
    uint3 dims;
    deltaSR.GetDimensions(dims.x, dims.y, dims.z); // all 3D inputs have the same dimensions

    float u = (float) groupId.x / dims.x;
    float v = (float) groupId.y / dims.y;

    uint tableWidth = dims.x;
    uint tableHeight = dims.y;

    uint layer = groupId.z;
    uint altLayers = dims.z + 1;

    float alt = layer / (altLayers - 1.0);
    alt = alt * alt;
    alt = sqrt(Rg * Rg + alt * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == altLayers - 1 ? -0.001 : 0.0));
    float4 dhdH = GetDhdH(alt);

    float3 muMusNu = GetMuMusNu(alt, dhdH, u, v, tableWidth, tableHeight, RES_NU);
    float3 raymie = Inscatter(alt, muMusNu.x, muMusNu.y, muMusNu.z, tableWidth, RES_NU);
    deltaJ[groupId] = float4(raymie, 1.0);
}
