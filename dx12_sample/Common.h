#pragma once

#ifdef __cplusplus
#include <DirectXMath.h>

#define float4 DirectX::XMVECTOR
#define float4x4 DirectX::XMMATRIX
#endif

struct ViewParams
{
    float4 viewPos;
    float4x4 inverseViewProj;
};
