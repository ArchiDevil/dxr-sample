#define TRANSMITTANCE_INTEGRAL_SAMPLES 16
#define TRANSMITTANCE_NON_LINEAR

#define INSCATTER_INTEGRAL_SAMPLES 16

#define Rt 6420 // top of atmosphere height
#define Rg 6360 // surface height

// Rayleigh scattering coefficients
#define HR 8.0
#define betaRSca float3(5.8e-3, 1.35e-2, 3.31e-2)
#define betaR betaRSca

// Mie scattering coefficients
#define HM 1.2
#define betaMSca float3(4e-3, 4e-3, 4e-3)
#define betaM (betaMSca / 0.9)

#define PI 3.14159265
#define PI_2 (PI / 2.0)

#define RES_NU 8 // resolution of the sun-zenith angle texture

int RaySphereIntersection(float alt, float mu, out float dist1, out float dist2)
{
    float2 rayPos = float2(0.0, alt);
    float2 rayDir = normalize(float2(sqrt(1 - mu * mu), mu));

    float p = dot(rayDir, rayPos);
    float q = dot(rayPos, rayPos) - (Rt * Rt);
    
    float discriminant = (p * p) - q;
    if (discriminant < 0.0f)
        return 0;

    float dRoot = sqrt(discriminant);
    dist1 = -p - dRoot;
    dist2 = -p + dRoot;

    return (discriminant > 1e-7) ? 2 : 1;
}

float IntersectAtmosphere(float alt, float mu)
{
    float dist1, dist2;
    int numIntersections = RaySphereIntersection(alt, mu, dist1, dist2);
    if (numIntersections == 0)
    {
        return 0.0f;
    }
    else if (numIntersections == 1)
    {
        return abs(dist2);
    }
    else
    {
        return max(dist1, dist2);
    }
}

// Returns the altitude and view-zenith angle for a given texture coordinates [0; 1]
float2 GetTransmittanceAltMu(float u, float v)
{
    float alt, mu;
    // non-linear transmittance
#ifdef TRANSMITTANCE_NON_LINEAR
    alt = v * v * (Rt - Rg) + Rg;
    mu = -0.15 + tan(1.5 * u) / tan(1.5) * (1.0 + 0.15);
#else
    // linear transmittance
    alt = Rg + u * (Rt - Rg);
    mu = -0.15 + v * (1.0 + 0.15);
#endif
    return float2(alt, mu);
}

// Get texture coordinate for a given altitude and mu
float2 GetTransmittanceUV(float alt, float mu)
{
    float u, v;
#ifdef TRANSMITTANCE_NON_LINEAR
    u = sqrt((alt - Rg) / (Rt - Rg));
    v = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
#else
    u = (alt - Rg) / (Rt - Rg);
    v = (mu + 0.15) / (1.0 + 0.15);
#endif
    return float2(u, v);
}

// Calculates the transmittance of the light for a given altitude and mu
float3 Transmittance(float alt, float mu, Texture2D transmittanceMap)
{
    float2 uv = GetTransmittanceUV(alt, mu);

    uint2 dims;
    transmittanceMap.GetDimensions(dims.x, dims.y);

    uint2 coords = uint2(uint(dims.x * uv.x), uint(dims.y * uv.y));
    return transmittanceMap[coords].rgb;
}

float2 GetIrradianceUV(float alt, float mus)
{
    float u = (alt - Rg) / (Rt - Rg);
    float v = (mus + 0.2) / (1.0 + 0.2);
    return float2(u, v);
}

float2 GetIrradianceAltMus(float u, float v)
{
    float alt = Rg + u * (Rt - Rg);
    float mus = -0.2 + v * (1.0 + 0.2);
    return float2(alt, mus);
}
