#define TRANSMITTANCE_INTEGRAL_SAMPLES 512
#define TRANSMITTANCE_NON_LINEAR

#define INSCATTER_INTEGRAL_SAMPLES 50
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
// #define INSCATTER_NON_LINEAR

#define IRRADIANCE_INTEGRAL_SAMPLES 32

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
#define AVERAGE_GROUND_REFLECTANCE 0.1

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
float3 Transmittance(float alt, float mu, RWTexture2D<float4> transmittance)
{
    float2 uv = GetTransmittanceUV(alt, mu);

    uint2 dims;
    transmittance.GetDimensions(dims.x, dims.y);

    uint2 coords = uint2(uint(dims.x * uv.x), uint(dims.y * uv.y));
    return transmittance[coords].rgb;
}

float3 Transmittance(float alt, float mu, float dist, RWTexture2D<float4> transmittance)
{
    float alt_i = sqrt(alt * alt + dist * dist + 2.0 * dist * alt * mu);
    return Transmittance(alt_i, mu, transmittance).rgb;
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

float3 Irradiance(RWTexture2D<float4> texture, float alt, float mus)
{
    float2 uv = GetIrradianceUV(alt, mus);
    uint2 dims;
    texture.GetDimensions(dims.x, dims.y);
    return texture[uint2(uint(uv.x * dims.x), uint(uv.y * dims.y))].rgb;
}

// Rayleigh phase function
float PhaseFunctionR(float mu)
{
    return (3.0 / (16.0 * PI)) * (1.0 + mu * mu);
}

// Mie phase function
float PhaseFunctionM(float mu)
{
    return 1.5 * 1.0 / (4.0 * PI) * (1.0 - HM * HM) * pow(1.0 + (HM * HM) - 2.0 * HM * mu, -3.0 / 2.0) * (1.0 + mu * mu) / (2.0 + HM * HM);
}

// approximated single Mie scattering
float3 GetMie(float4 rayMie)
{
    // rayMie.rgb=C*, rayMie.w=Cm,r
    return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
}

float4 SampleTexture4D(RWTexture3D<float4> texture, float alt, float mu, float mus, float nu, uint tableWidth, uint resNu)
{
    uint3 dims;
    texture.GetDimensions(dims.x, dims.y, dims.z);
    const float resMus = tableWidth / resNu;

    float H = sqrt(Rt * Rt - Rg * Rg);
    float rho = sqrt(alt * alt - Rg * Rg);
#ifdef INSCATTER_NON_LINEAR
    float rmu = alt * mu;
    float delta = rmu * rmu - alt * alt + Rg * Rg;
    float4 cst = rmu < 0.0 && delta > 0.0 ? float4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(dims.y)) : float4(-1.0, H * H, H, 0.5 + 0.5 / float(dims.y));
    float uR = 0.5 / float(dims.z) + rho / H * (1.0 - 1.0 / float(dims.z));
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(dims.y));
    // paper formula
    //float uMuS = 0.5 / float(resMus) + max((1.0 - exp(-3.0 * mus - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(resMus));
    // better formula
    float uMuS = 0.5 / float(resMus) + (atan(max(mus, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(resMus));
#else
    float uR = 0.5 / float(dims.z) + rho / H * (1.0 - 1.0 / float(dims.z));
    float uMu = 0.5 / float(dims.y) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(dims.y));
    float uMuS = 0.5 / float(resMus) + max(mus + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(resMus));
#endif
    float lerp = (nu + 1.0) / 2.0 * (float(resNu) - 1.0);
    float uNu = floor(lerp);
    lerp = lerp - uNu;
    
    uint3 coords1 = uint3(uint(dims.x * (uNu + uMuS) / float(resNu)), uint(dims.y * uMu), uint(dims.z * uR));
    uint3 coords2 = uint3(uint(dims.x * (uNu + uMuS + 1.0) / float(resNu)), uint(dims.y * uMu), uint(dims.z * uR));

    return texture[coords1] * (1.0 - lerp) + texture[coords2] * lerp;
}

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float3 GetMuMusNu(float alt, float4 dhdH, float u, float v, uint tableWidth, uint tableHeight, uint resNu)
{
    float x = u * float(tableWidth);
    float y = v * tableHeight;
    float resMus = tableWidth / resNu;
    
    float mu, mus, nu;

#ifdef INSCATTER_NON_LINEAR
    if (y < float(tableHeight) / 2.0)
    {
        float d = 1.0 - y / (float(tableHeight) / 2.0 - 1.0);
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999);
        mu = (Rg * Rg - alt * alt - d * d) / (2.0 * alt * d);
        mu = min(mu, -sqrt(1.0 - (Rg / alt) * (Rg / alt)) - 0.001);
    }
    else
    {
        float d = (y - float(tableHeight) / 2.0) / (float(tableHeight) / 2.0 - 1.0);
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999);
        mu = (Rt * Rt - alt * alt - d * d) / (2.0 * alt * d);
    }
    mus = mod(x, float(resMus)) / (float(resMus) - 1.0);
    // paper formula
    //mus = -(0.6 + log(1.0 - mus * (1.0 -  exp(-3.6)))) / 3.0;
    // better formula
    mus = tan((2.0 * mus - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
    nu = -1.0 + floor(x / float(resMus)) / (float(resNu) - 1.0) * 2.0;
#else
    mu = -1.0 + 2.0 * y / (float(tableHeight) - 1.0);
    mus = mod(x, resMus) / (resMus - 1.0);
    mus = -0.2 + mus * 1.2;
    nu = -1.0 + floor(x / resMus) / (float(resNu) - 1.0) * 2.0;
#endif

    return float3(mu, mus, nu);
}

float4 GetDhdH(float alt)
{
    float dmin = Rt - alt;
    float dmax = sqrt(alt * alt - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
    float dminp = alt - Rg;
    float dmaxp = sqrt(alt * alt - Rg * Rg);
    return float4(dmin, dmax, dminp, dmaxp);
}
