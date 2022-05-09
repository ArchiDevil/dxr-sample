#define TRANSMITTANCE_INTEGRAL_SAMPLES 16
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
