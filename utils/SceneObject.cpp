#include "stdafx.h"

#include "SceneObject.h"

#include "Types.h"

#include <shaders/Common.h>

SceneObject::SceneObject(std::shared_ptr<MeshObject> meshObject,
                         ComPtr<ID3D12Device> pDevice)
    : _meshObject(meshObject)
    , _device(pDevice)
{
    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = sizeof(ModelParams);
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    pDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_constantBuffer));

    _color = { float(rand() % 50 + 50.0f) / 100, float(rand() % 50 + 50.0f) / 100, float(rand() % 50 + 50.0f) / 100, 1 };
}

bool SceneObject::IsDirty() const
{
    return _transformDirty;
}

void SceneObject::ResetDirty()
{
    _transformDirty = false;
}

const XMMATRIX& SceneObject::GetWorldMatrix() const
{
    return _worldMatrix;
}

const ComPtr<ID3D12Resource>& SceneObject::GetBLAS() const 
{
    return _meshObject->BLAS();
}

const ComPtr<ID3D12Resource>& SceneObject::GetVertexBuffer() const
{
     return _meshObject->VertexBuffer();
}
const ComPtr<ID3D12Resource>& SceneObject::GetIndexBuffer() const
{
    return _meshObject->IndexBuffer();
}

DirectX::XMFLOAT3 SceneObject::Position() const
{
    return _position;
}

void SceneObject::Position(DirectX::XMFLOAT3 val)
{
    _position = val;
    _transformDirty = true;
    CalculateWorldMatrix();
}

DirectX::XMFLOAT3 SceneObject::Scale() const
{
    return _scale;
}

void SceneObject::Scale(DirectX::XMFLOAT3 val)
{
    _scale = val;
    _transformDirty = true;
    CalculateWorldMatrix();
}

float SceneObject::Rotation() const
{
    return _rotation;
}

void SceneObject::Rotation(float val)
{
    _rotation = val;
    _transformDirty = true;
    CalculateWorldMatrix();
}

ComPtr<ID3D12Resource> SceneObject::GetConstantBuffer() const
{
    return _constantBuffer;
}

void SceneObject::CalculateWorldMatrix()
{
    XMMATRIX translationMatrix = XMMatrixTranslation(_position.x, _position.y, _position.z);
    XMMATRIX rotationMatrix = XMMatrixRotationX(_rotation);
    XMMATRIX scaleMatrix = XMMatrixScaling(_scale.x, _scale.y, _scale.z);

    _worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
    _worldMatrix = XMMatrixTranspose(_worldMatrix);

    ModelParams* bufPtr = nullptr;
    ModelParams  params;
    params.color = XMVECTOR{_color.x, _color.y, _color.z, _color.w};
    params.worldMatrix = _worldMatrix;

    ThrowIfFailed(_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
    memcpy(bufPtr, &params, sizeof(ModelParams));
    _constantBuffer->Unmap(0, nullptr);
}
