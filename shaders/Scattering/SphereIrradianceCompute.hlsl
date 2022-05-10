#include "Common.hlsl"

RWTexture3D<float4> deltaSM : register(u0);
RWTexture3D<float4> deltaSR : register(u1);
RWTexture2D<float4> deltaE  : register(u2);

uint first : register(c0);

[numthreads(1, 1, 1)]
void CSMain(uint2 groupId : SV_GroupID)
{
    uint2 dims;
    deltaE.GetDimensions(dims.x, dims.y);
    
    uint3 deltaDims;
    deltaSM.GetDimensions(deltaDims.x, deltaDims.y, deltaDims.z);
    
    float u = groupId.x / float(dims.x);
    float v = groupId.y / float(dims.y);
    
    const float dphi = PI / float(IRRADIANCE_INTEGRAL_SAMPLES);
    const float dtheta = PI / float(IRRADIANCE_INTEGRAL_SAMPLES);
    const float resNu = RES_NU;

    float2 altMus = GetIrradianceAltMus(u, v);
    float3 s = float3(max(sqrt(1.0 - altMus.y * altMus.y), 0.0), 0.0, altMus.y);

    float3 result = float3(0, 0, 0);
    // integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15)
    for (int iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi)
    {
        float phi = (float(iphi) + 0.5) * dphi;
        for (int itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta)
        {
            float theta = (float(itheta) + 0.5) * dtheta;
            float dw = dtheta * dphi * sin(theta);
            float3 w = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
            float nu = dot(s, w);
            if (first == 1)
            {
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = PhaseFunctionR(nu);
                float pm1 = PhaseFunctionM(nu);

                float3 ray1 = SampleTexture4D(deltaSR, altMus.x, w.z, altMus.y, nu, deltaDims.x, resNu).rgb;
                float3 mie1 = SampleTexture4D(deltaSM, altMus.x, w.z, altMus.y, nu, deltaDims.x, resNu).rgb;
                result += (ray1 * pr1 + mie1 * pm1) * w.z * dw;
            }
            else
            {
                result += SampleTexture4D(deltaSR, altMus.x, w.z, altMus.y, nu, deltaDims.x, resNu).rgb * w.z * dw;
            }
        }
    }
    
    deltaE[groupId] = float4(result, 0.0);
}
