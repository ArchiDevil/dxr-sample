#include "stdafx.h"

#include "SceneObject.h"

#include "Types.h"

#include <shaders/Common.h>

SceneObject::SceneObject(std::shared_ptr<MeshObject> meshObject,
                         DescriptorHeap&             heap,
                         ComPtr<ID3D12Device>        device,
                         Material                    material /*= Material(MaterialType::Diffuse)*/)
    : _meshObject(meshObject)
    , _device(device)
    , _material(material)
{
    assert(device);
    assert(meshObject);

    CreateBufferSRVs(heap);
    CreateCBV();
    CalculateWorldMatrix();
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

const MeshObject& SceneObject::GetMeshObject() const
{
    return *_meshObject;
}

Material& SceneObject::GetMaterial()
{
    return _material;
}

std::size_t SceneObject::GetDescriptorIdx() const
{
    return _descriptorIdx;
}

DirectX::XMFLOAT3 SceneObject::Position() const
{
    return _position;
}

void SceneObject::Position(DirectX::XMFLOAT3 val)
{
    _position       = val;
    _transformDirty = true;
    CalculateWorldMatrix();
}

DirectX::XMFLOAT3 SceneObject::Scale() const
{
    return _scale;
}

void SceneObject::Scale(DirectX::XMFLOAT3 val)
{
    _scale          = val;
    _transformDirty = true;
    CalculateWorldMatrix();
}

float SceneObject::Rotation() const
{
    return _rotation;
}

void SceneObject::Rotation(float val)
{
    _rotation       = val;
    _transformDirty = true;
    CalculateWorldMatrix();
}

ComPtr<ID3D12Resource> SceneObject::GetConstantBuffer() const
{
    return _constantBuffer;
}

void SceneObject::CreateBufferSRVs(DescriptorHeap& heap)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    // common properties
    viewDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Buffer.FirstElement     = 0;

    // vertex buffer SRV
    viewDesc.Format                     = DXGI_FORMAT_UNKNOWN;
    viewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
    viewDesc.Buffer.StructureByteStride = sizeof(GeometryVertex);
    viewDesc.Buffer.NumElements         = _meshObject->VerticesCount();

    auto freeAddress = heap.GetFreeCPUAddress();
    _descriptorIdx   = freeAddress.index;
    _device->CreateShaderResourceView(_meshObject->VertexBuffer().Get(), &viewDesc, freeAddress.handle);

    // index buffer SRV
    viewDesc.Format                     = DXGI_FORMAT_R32_TYPELESS;
    viewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
    viewDesc.Buffer.StructureByteStride = 0;
    viewDesc.Buffer.NumElements         = _meshObject->IndicesCount();

    freeAddress = heap.GetFreeCPUAddress();
    _device->CreateShaderResourceView(_meshObject->IndexBuffer().Get(), &viewDesc, freeAddress.handle);
}

void SceneObject::CreateCBV()
{
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width               = sizeof(ModelParams);
    constantBufferDesc.Height              = 1;
    constantBufferDesc.MipLevels           = 1;
    constantBufferDesc.SampleDesc.Count    = 1;
    constantBufferDesc.DepthOrArraySize    = 1;
    constantBufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};
    _device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_constantBuffer));
}

void SceneObject::CalculateWorldMatrix()
{
    XMMATRIX translationMatrix = XMMatrixTranslation(_position.x, _position.y, _position.z);
    XMMATRIX rotationMatrix    = XMMatrixRotationX(_rotation);
    XMMATRIX scaleMatrix       = XMMatrixScaling(_scale.x, _scale.y, _scale.z);

    _worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
    // TODO(DB): it is not needed here, transposing is only required for TLAS building
    // remove it and make transposing in TLAS building only, it reduces performance
    _worldMatrix = XMMatrixTranspose(_worldMatrix);

    ModelParams params;
    params.worldMatrix = _worldMatrix;

    if (_material.GetType() == MaterialType::Specular)
    {
        SpecularMaterial mtl   = std::get<SpecularMaterial>(_material.GetParams());
        auto             color = mtl.color;
        params.reflectance     = mtl.reflectance;
    }
    else if (_material.GetType() == MaterialType::Diffuse)
    {
        // no params yet
    }

    ModelParams* bufPtr = nullptr;
    ThrowIfFailed(_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
    memcpy(bufPtr, &params, sizeof(ModelParams));
    _constantBuffer->Unmap(0, nullptr);
}
