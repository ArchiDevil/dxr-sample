#pragma once

#ifdef __cplusplus
#include <DirectXMath.h>

#define float4 DirectX::XMVECTOR
#define float4x4 DirectX::XMMATRIX
#endif

struct ViewParams
{
    float4x4 inverseViewProj;
    float4 viewPos;
};

struct LightParams
{
    float4 direction;
    float4 color;
};
