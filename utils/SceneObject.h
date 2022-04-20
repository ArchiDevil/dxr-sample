#pragma once

#include "stdafx.h"

#include <utils/MeshManager.h>

__declspec(align(16)) class SceneObject
{
public:
    SceneObject(std::shared_ptr<MeshObject> meshObject,
                ComPtr<ID3D12Device> pDevice);

    DirectX::XMFLOAT3 Position() const;
    void Position(DirectX::XMFLOAT3 val);

    DirectX::XMFLOAT3 Scale() const;
    void Scale(DirectX::XMFLOAT3 val);

    float Rotation() const;
    void Rotation(float val);

    void * operator new(size_t i)
    {
        return _aligned_malloc(i, 16);
    }

    void operator delete(void* p)
    {
        _aligned_free(p);
    }

    ComPtr<ID3D12Resource> GetConstantBuffer() const;
    const XMMATRIX& GetWorldMatrix() const;
    const ComPtr<ID3D12Resource>& GetBLAS() const;

    const ComPtr<ID3D12Resource>& GetVertexBuffer() const;
    const ComPtr<ID3D12Resource>& GetIndexBuffer() const;

    bool IsDirty() const;
    void ResetDirty();

private:
    void CalculateWorldMatrix();

    std::shared_ptr<MeshObject>         _meshObject = nullptr;

    XMFLOAT3                            _position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3                            _scale = {1.0f, 1.0f, 1.0f};
    float                               _rotation = 0.0f;
    XMMATRIX                            _worldMatrix = XMMatrixIdentity();
    XMFLOAT4                            _color;

    ComPtr<ID3D12Resource>              _constantBuffer = nullptr;
    ComPtr<ID3D12Resource>              _blas = nullptr;
    ComPtr<ID3D12Device>                _device = nullptr;

    bool                                _transformDirty = true;
};
