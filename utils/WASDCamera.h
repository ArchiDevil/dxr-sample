#pragma once

#include "AbstractCamera.h"

namespace Graphics
{
class WASDCamera : public AbstractCamera
{
public:
    WASDCamera(float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight);

    void SetPosition(XMFLOAT3 position);

    void MoveForward(float delta);
    void MoveUpward(float delta);
    void MoveSideward(float delta);

    void RotateHorizontally(float deltaAngle);
    void RotateVertically(float deltaAngle);

private:
    void UpdateRotation();

    float _horizontalAngle = 0.0f;
    float _verticalAngle   = 0.0f;
};
}  // namespace Graphics
