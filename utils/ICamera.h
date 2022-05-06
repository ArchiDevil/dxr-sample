#pragma once

#include "stdafx.h"

struct ICamera
{
    virtual ~ICamera() = default;

    virtual void SetScreenParams(float screenWidth, float screenHeight) = 0;

    virtual XMMATRIX GetViewProjMatrix() const = 0;
    virtual XMFLOAT3 GetPosition() const       = 0;
};
