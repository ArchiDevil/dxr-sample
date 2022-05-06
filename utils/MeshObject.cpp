#include "stdafx.h"

#include "MeshObject.h"

void CreateUAVBuffer(ComPtr<ID3D12Device5> device, size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState)
{
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width               = bufferSize;
    bufferDesc.Height              = 1;
    bufferDesc.MipLevels           = 1;
    bufferDesc.SampleDesc.Count    = 1;
    bufferDesc.DepthOrArraySize    = 1;
    bufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_DEFAULT};
    ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialState, nullptr,
                                                  IID_PPV_ARGS(&(*pOutBuffer))));
}

MeshObject::MeshObject(std::span<const uint8_t>          vertexData,
                       std::size_t                       stride,
                       std::span<const uint32_t>         indexData,
                       ComPtr<ID3D12Device5>             device,
                       bool                              createBlas,
                       std::function<void(CommandList&)> cmdListExecutor)
    : _indicesCount(indexData.size())
    , _verticesCount(vertexData.size() / stride)
{
    D3D12_HEAP_PROPERTIES heapProp = { D3D12_HEAP_TYPE_UPLOAD };

    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Width = vertexData.size();
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.SampleDesc.Count = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer)));

    void* bufPtr = nullptr;
    ThrowIfFailed(_vertexBuffer->Map(0, nullptr, &bufPtr));
    std::memcpy(bufPtr, vertexData.data(), vertexData.size());
    _vertexBuffer->Unmap(0, nullptr);

    // creating view describing how to use vertex buffer for GPU
    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes    = (UINT)vertexData.size();
    _vertexBufferView.StrideInBytes  = (UINT)stride;

    if (!indexData.empty())
    {
        D3D12_RESOURCE_DESC indexBufferDesc = {};
        indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indexBufferDesc.Width = indexData.size() * sizeof(uint32_t);
        indexBufferDesc.Height = 1;
        indexBufferDesc.MipLevels = 1;
        indexBufferDesc.SampleDesc.Count = 1;
        indexBufferDesc.DepthOrArraySize = 1;
        indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_indexBuffer)));

        ThrowIfFailed(_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
        std::memcpy(bufPtr, indexData.data(), indexData.size() * sizeof(uint32_t));
        _indexBuffer->Unmap(0, nullptr);

        // creating view describing how to use vertex buffer for GPU
        _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
        _indexBufferView.SizeInBytes    = (UINT)(indexData.size() * sizeof(uint32_t));
        _indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
    }

    //////////////////////////////////////////////////////////////////////////

    if (!createBlas)
        return;

    assert(cmdListExecutor);

    CommandList cmdList{ CommandListType::Direct, device };

    D3D12_RAYTRACING_GEOMETRY_DESC geoDesc = {};
    geoDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geoDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geoDesc.Triangles.VertexBuffer.StartAddress = _vertexBuffer->GetGPUVirtualAddress();
    geoDesc.Triangles.VertexBuffer.StrideInBytes = (UINT)stride;
    geoDesc.Triangles.VertexCount = _verticesCount;
    geoDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;  // TODO (DB): understand what it is
    geoDesc.Triangles.IndexBuffer = _indexBuffer->GetGPUVirtualAddress();
    geoDesc.Triangles.IndexCount = _indicesCount;
    geoDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
    auto& inputs = blasDesc.Inputs;

    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    // TODO (DB): check what flags we have and how we could use it
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geoDesc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
        throw std::runtime_error("Zeroed size");

    ComPtr<ID3D12Resource> scratchResource;
    CreateUAVBuffer(device, prebuildInfo.ScratchDataSizeInBytes, &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    CreateUAVBuffer(device, prebuildInfo.ResultDataMaxSizeInBytes, &_blas,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
    _blas->SetName(L"Bottom-Level Acceleration Structure");

    {
        blasDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        blasDesc.DestAccelerationStructureData = _blas->GetGPUVirtualAddress();
    }

    cmdList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
    cmdList.Close();

    cmdListExecutor(cmdList);
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& MeshObject::VertexBuffer() const
{
    return _vertexBuffer;
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& MeshObject::IndexBuffer() const
{
    return _indexBuffer;
}

const D3D12_VERTEX_BUFFER_VIEW& MeshObject::VertexBufferView() const
{
    return _vertexBufferView;
}

const D3D12_INDEX_BUFFER_VIEW& MeshObject::IndexBufferView() const
{
    return _indexBufferView;
}

const ComPtr<ID3D12Resource>& MeshObject::BLAS() const
{
    return _blas;
}

size_t MeshObject::VerticesCount() const
{
    return _verticesCount;
}

size_t MeshObject::IndicesCount() const
{
    return _indicesCount;
}
