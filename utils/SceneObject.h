#pragma once

#include "stdafx.h"

#include <utils/DescriptorHeap.h>
#include <utils/MeshManager.h>

#include <variant>

enum class MaterialType
{
    Diffuse,
    Specular
};

struct DiffuseMaterial
{
    XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
};

struct SpecularMaterial
{
    XMFLOAT3 color       = {1.0f, 1.0f, 1.0f};
    float    reflectance = 10.0f;
};

class Material
{
public:
    Material(MaterialType type)
        : _type(type)
    {
        switch (type)
        {
        case MaterialType::Diffuse:
            _params.emplace<DiffuseMaterial>();
            break;
        case MaterialType::Specular:
            _params.emplace<SpecularMaterial>();
            break;
        default:
            assert(false);
            break;
        }
    }

    auto& GetParams()
    {
        return _params;
    }

    MaterialType GetType() const
    {
        return _type;
    }

private:
    MaterialType                                    _type;
    std::variant<DiffuseMaterial, SpecularMaterial> _params;
};

class alignas(16) SceneObject
{
public:
    SceneObject(std::shared_ptr<MeshObject> meshObject,
                DescriptorHeap&             heap,
                ComPtr<ID3D12Device>        pDevice,
                Material                    material = Material(MaterialType::Diffuse));

    DirectX::XMFLOAT3 Position() const;
    void              Position(DirectX::XMFLOAT3 val);

    DirectX::XMFLOAT3 Scale() const;
    void              Scale(DirectX::XMFLOAT3 val);

    float Rotation() const;
    void  Rotation(float val);

    ComPtr<ID3D12Resource> GetConstantBuffer() const;
    const XMMATRIX&        GetWorldMatrix() const;
    const MeshObject&      GetMeshObject() const;
    Material&              GetMaterial();

    bool IsDirty() const;
    void ResetDirty();

    std::size_t GetDescriptorIdx() const;

private:
    void CreateBufferSRVs(DescriptorHeap& heap);
    void CreateCBV();
    void CalculateWorldMatrix();

    std::shared_ptr<MeshObject> _meshObject = nullptr;

    XMFLOAT3 _position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 _scale    = {1.0f, 1.0f, 1.0f};
    float    _rotation = 0.0f;

    XMMATRIX _worldMatrix = XMMatrixIdentity();
    Material _material;

    ComPtr<ID3D12Resource> _constantBuffer = nullptr;
    ComPtr<ID3D12Resource> _blas           = nullptr;
    ComPtr<ID3D12Device>   _device         = nullptr;

    bool        _transformDirty = true;
    std::size_t _descriptorIdx;
};
