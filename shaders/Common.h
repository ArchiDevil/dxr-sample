#pragma once

#ifdef __cplusplus
#    include <DirectXMath.h>

#    define float2   DirectX::XMFLOAT2
#    define float3   DirectX::XMFLOAT3
#    define float4   DirectX::XMVECTOR
#    define float4x4 DirectX::XMMATRIX
#endif

#ifndef __cplusplus
#    define alignas(x)
#endif

struct ViewParams
{
    float4x4 inverseViewProj;
    float4   viewPos;
    float4   ambientColor;
};

struct LightParams
{
    float4 direction;
    float4 color;
};

struct ModelParams
{
    float4   color;
    float4x4 worldMatrix;
    float reflectance;
};

struct GeometryVertex
{
    float3 position;
    float3 normal;
    float3 color;
};
