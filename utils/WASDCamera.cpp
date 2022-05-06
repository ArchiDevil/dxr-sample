#include "WASDCamera.h"

#include <cmath>

namespace Graphics
{
constexpr float pi   = 3.14159265358979323846f;
constexpr float pi_2 = pi / 2.0f;

WASDCamera::WASDCamera(float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight)
    : AbstractCamera(nearPlane, farPlane, fov, screenWidth, screenHeight)
{
    UpdateRotation();
}

void WASDCamera::SetPosition(XMFLOAT3 position)
{
    AbstractCamera::SetTriada(position, AbstractCamera::GetLookAt(), AbstractCamera::GetUp());
}

void WASDCamera::MoveForward(float delta)
{
    auto lookAt = GetLookAt();
    lookAt.x *= delta;
    lookAt.y *= delta;
    lookAt.z *= delta;

    auto pos      = AbstractCamera::GetPosition();
    auto movedPos = XMVectorAdd(XMLoadFloat3(&lookAt), XMLoadFloat3(&pos));
    XMStoreFloat3(&pos, movedPos);

    AbstractCamera::SetTriada(pos, AbstractCamera::GetLookAt(), AbstractCamera::GetUp());
}

void WASDCamera::MoveUpward(float delta)
{
    auto up = AbstractCamera::GetUp();
    up.x *= delta;
    up.y *= delta;
    up.z *= delta;

    auto pos      = AbstractCamera::GetPosition();
    auto movedPos = XMVectorAdd(XMLoadFloat3(&up), XMLoadFloat3(&pos));
    XMStoreFloat3(&pos, movedPos);

    AbstractCamera::SetTriada(pos, AbstractCamera::GetLookAt(), AbstractCamera::GetUp());
}

void WASDCamera::MoveSideward(float delta)
{
    auto lookAt = AbstractCamera::GetLookAt();
    auto up     = AbstractCamera::GetUp();

    auto right = XMVector3Cross(XMLoadFloat3(&lookAt), XMLoadFloat3(&up));
    XMFLOAT3 storedRight;
    XMStoreFloat3(&storedRight, right);
    storedRight.x *= delta;
    storedRight.y *= delta;
    storedRight.z *= delta;

    auto pos      = AbstractCamera::GetPosition();
    auto movedPos = XMVectorAdd(XMLoadFloat3(&storedRight), XMLoadFloat3(&pos));
    XMStoreFloat3(&pos, movedPos);

    AbstractCamera::SetTriada(pos, AbstractCamera::GetLookAt(), AbstractCamera::GetUp());
}

void WASDCamera::RotateHorizontally(float deltaAngle)
{
    _horizontalAngle += deltaAngle;
    UpdateRotation();
}

void WASDCamera::RotateVertically(float deltaAngle)
{
    _verticalAngle = std::clamp(_verticalAngle + deltaAngle, -pi_2 * 0.95f, pi_2 * 0.95f);
    UpdateRotation();
}

void WASDCamera::UpdateRotation()
{
    auto lookAt = AbstractCamera::GetDefaultLookAt();
    auto up     = AbstractCamera::GetUp();

    auto loadedLook = XMLoadFloat3(&lookAt);
    auto loadedUp   = XMLoadFloat3(&up);
    auto right      = XMVector3Cross(loadedLook, loadedUp);

    auto quaternion  = XMQuaternionRotationAxis(right, _verticalAngle);
    auto rotatedLook = XMVector3Rotate(loadedLook, quaternion);

    quaternion  = XMQuaternionRotationAxis(loadedUp, _horizontalAngle);
    rotatedLook = XMVector3Rotate(rotatedLook, quaternion);

    XMStoreFloat3(&lookAt, rotatedLook);

    AbstractCamera::SetTriada(GetPosition(), lookAt, up);
}
}  // namespace Graphics
