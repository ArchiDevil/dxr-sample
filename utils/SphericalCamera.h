#pragma once

#include "stdafx.h"

#include "AbstractCamera.h"

namespace Graphics
{
struct CameraPosition
{
    XMFLOAT3 centerPosition;
    float    radius;
    float    rotation;
    float    inclination;
};

class SphericalCamera : public AbstractCamera
{
public:
    SphericalCamera(float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight);

    void SetCenter(const XMFLOAT3& position);
    void SetRadius(float radius);
    void SetInclination(float inclination);
    void SetRotation(float rotation);

    CameraPosition GetCameraPosition() const;

private:
    void CalculateTriada();

    float _radius      = 10.0f;
    float _rotation    = 0.0f;
    float _inclination = 0.0f;
};
}  // namespace Graphics
