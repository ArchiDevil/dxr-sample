#pragma once

#ifdef __cplusplus
#include <DirectXMath.h>

#define float4 DirectX::XMVECTOR
#endif

struct ViewParams
{
    float4 viewPos;
};
