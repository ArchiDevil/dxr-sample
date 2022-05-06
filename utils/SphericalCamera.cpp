#include "stdafx.h"

#include "SphericalCamera.h"

#include "Math.h"

namespace Graphics
{
SphericalCamera::SphericalCamera(float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight)
    : AbstractCamera(nearPlane, farPlane, fov, screenWidth, screenHeight)
{
    CalculateTriada();
}

void SphericalCamera::SetCenter(const XMFLOAT3& position)
{
    SetTriada(position, GetLookAt(), GetUp());
    CalculateTriada();
}

void SphericalCamera::SetRadius(float radius)
{
    _radius = radius;
    CalculateTriada();
}

void SphericalCamera::SetInclination(float inclination)
{
    _inclination = inclination;
    CalculateTriada();
}

void SphericalCamera::SetRotation(float rotation)
{
    _rotation = rotation;
    CalculateTriada();
}

CameraPosition SphericalCamera::GetCameraPosition() const
{
    return {GetPosition(), _radius, _rotation, _inclination};
}

void SphericalCamera::CalculateTriada()
{
    XMFLOAT4 position = Math::GetPointOnSphere(GetPosition(), _radius, _rotation, _inclination);
    AbstractCamera::SetTriada({position.x, position.y, position.z}, GetPosition(), {0.0f, 0.0f, 1.0f});
}
}  // namespace Graphics
