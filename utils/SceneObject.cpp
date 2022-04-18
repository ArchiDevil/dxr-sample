#include "stdafx.h"

#include "SceneObject.h"

#include "Types.h"

SceneObject::SceneObject(std::shared_ptr<MeshObject> meshObject,
                         ComPtr<ID3D12Device> pDevice)
    : _meshObject(meshObject)
    , _device(pDevice)
{
    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = sizeof(perModelParamsConstantBuffer);
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    pDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_constantBuffer));
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

    //perModelParamsConstantBuffer * bufPtr = nullptr;
    //ThrowIfFailed(_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
    //memcpy(bufPtr->worldMatrix, GetWorldMatrix().r, sizeof(XMMATRIX));
    //_constantBuffer->Unmap(0, nullptr);
}
