#pragma once

#include "ICamera.h"

namespace Graphics
{
class AbstractCamera : public ICamera
{
public:
    AbstractCamera(float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight)
        : _zNear(nearPlane)
        , _zFar(farPlane)
        , _fov(fov)
        , _screenWidth(screenWidth)
        , _screenHeight(screenHeight)
    {
        BuildProjMatrix();
        BuildViewMatrix();
    }

    // Inherited by ICamera
    void SetScreenParams(float screenWidth, float screenHeight) override
    {
        _screenWidth  = screenWidth;
        _screenHeight = screenHeight;
        BuildProjMatrix();
    }

    XMMATRIX GetViewProjMatrix() const override
    {
        return _viewMatrix * _projectionMatrix;
    }

    XMFLOAT3 GetPosition() const override
    {
        return {_eyePos.x, _eyePos.y, _eyePos.z};
    }

protected:
    void SetTriada(XMFLOAT3 eyePos, XMFLOAT3 lookAt, XMFLOAT3 up)
    {
        _eyePos = eyePos;
        _lookAt = lookAt;
        _up     = up;
        BuildViewMatrix();
    }

    XMFLOAT3 GetDefaultLookAt() const
    {
        return {0.0f, 1.0f, 0.0f};
    }

    XMFLOAT3 GetLookAt() const
    {
        return _lookAt;
    }

    XMFLOAT3 GetUp() const
    {
        return _up;
    }

private:
    float GetAspectRatio() const
    {
        return _screenWidth / _screenHeight;
    }

    void BuildViewMatrix()
    {
        XMVECTOR pos  = XMLoadFloat3(&_eyePos);
        XMVECTOR look = XMLoadFloat3(&_lookAt);
        XMVECTOR up   = XMLoadFloat3(&_up);

        XMVECTOR adjustedLook = XMVectorAdd(pos, look);
        _viewMatrix           = XMMatrixLookAtLH(pos, adjustedLook, up);
    }

    void BuildProjMatrix()
    {
        _projectionMatrix = XMMatrixPerspectiveFovLH(_fov, GetAspectRatio(), _zNear, _zFar);
    }

    float _zNear;
    float _zFar;
    float _fov;
    float _screenWidth;
    float _screenHeight;

    XMFLOAT3 _eyePos = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 _lookAt = GetDefaultLookAt();
    XMFLOAT3 _up{0.0f, 0.0f, 1.0f};

    XMMATRIX _viewMatrix       = XMMatrixIdentity();
    XMMATRIX _projectionMatrix = XMMatrixIdentity();
};
}  // namespace Graphics
