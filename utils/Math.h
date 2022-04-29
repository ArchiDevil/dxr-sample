#pragma once

#include "stdafx.h"

namespace Math
{
constexpr std::size_t AlignTo(std::size_t value, std::size_t alignment)
{
    // This method aligns the value to the nearest multiple of the alignment.
    return (value + alignment - 1) & ~(alignment - 1);
}

inline DirectX::XMFLOAT4 GetPointOnSphere(const XMFLOAT3& center, float radius, float rotation, float inclination)
{
    float x = center.x + radius * std::sin(rotation * 0.0175f) * std::cos(inclination * 0.0175f);
    float y = center.y + radius * std::cos(rotation * 0.0175f) * std::cos(inclination * 0.0175f);

    float z = center.z + radius * std::sin(inclination * 0.0175f);
    return {x, y, z, 1.0f};
}

}  // namespace Math
