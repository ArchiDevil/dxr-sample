#include "stdafx.h"

#include "MeshManager.h"

#include "Types.h"

static const std::vector<geometryVertex> vertices =
{
    // back face +Z
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // front face -Z
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // bottom face -Y
    {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},

    // top face +Y
    {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

    // left face -X
    {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

    // right face +X
    {{0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},

    // back faces

    // back face +Z
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // front face -Z
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // bottom face -Y
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},

    // top face +Y
    {{-0.5f,  0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

    // left face -X
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

    // right face +X
    {{0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
};

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

MeshObject::MeshObject(const std::vector<uint8_t>&       vertex_data,
                       size_t                            stride,
                       const std::vector<uint32_t>&      index_data,
                       ComPtr<ID3D12Device5>             device,
                       bool                              createBlas/*      = true*/,
                       std::function<void(CommandList&)> cmdListExecutor/* = {}*/)
    : _indicesCount(index_data.size())
    , _verticesCount(vertex_data.size() / stride)
{
    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};

    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Width = vertex_data.size();
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.SampleDesc.Count = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer)));

    void * bufPtr = nullptr;
    ThrowIfFailed(_vertexBuffer->Map(0, nullptr, &bufPtr));
    std::memcpy(bufPtr, vertex_data.data(), vertex_data.size());
    _vertexBuffer->Unmap(0, nullptr);

    if (!index_data.empty())
    {
        D3D12_RESOURCE_DESC indexBufferDesc = {};
        indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indexBufferDesc.Width = index_data.size() * sizeof(uint32_t);
        indexBufferDesc.Height = 1;
        indexBufferDesc.MipLevels = 1;
        indexBufferDesc.SampleDesc.Count = 1;
        indexBufferDesc.DepthOrArraySize = 1;
        indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_indexBuffer)));

        ThrowIfFailed(_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
        std::memcpy(bufPtr, index_data.data(), index_data.size() * sizeof(uint32_t));
        _indexBuffer->Unmap(0, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////

    if (!createBlas)
        return;

    assert(cmdListExecutor);

    CommandList cmdList{CommandListType::Direct, device };

    D3D12_RAYTRACING_GEOMETRY_DESC geoDesc = {};
    geoDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geoDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geoDesc.Triangles.VertexBuffer.StartAddress = _vertexBuffer->GetGPUVirtualAddress();
    geoDesc.Triangles.VertexBuffer.StrideInBytes = (UINT)stride;
    geoDesc.Triangles.VertexCount = _verticesCount;
    geoDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; // TODO (DB): understand what it is
    geoDesc.Triangles.IndexBuffer = _indexBuffer->GetGPUVirtualAddress();
    geoDesc.Triangles.IndexCount = _indicesCount;
    geoDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
    auto&                                              inputs   = blasDesc.Inputs;

    inputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    // TODO (DB): check what flags we have and how we could use it
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    // TODO (DB): update to the correct number of objects
    inputs.NumDescs       = 1;
    inputs.pGeometryDescs = &geoDesc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
        throw std::runtime_error("Zeroed size");

    ComPtr<ID3D12Resource> scratchResource;
    CreateUAVBuffer(device, prebuildInfo.ScratchDataSizeInBytes, &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    CreateUAVBuffer(device, prebuildInfo.ResultDataMaxSizeInBytes, &_blas, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
    _blas->SetName(L"Bottom-Level Acceleration Structure");

    {
        blasDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        blasDesc.DestAccelerationStructureData    = _blas->GetGPUVirtualAddress();
    }

    cmdList.GetInternal()->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
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

//////////////////////////////////////////////////////////////////////////

MeshManager::MeshManager(ComPtr<ID3D12Device5> device)
    : _device(device)
{
}

std::shared_ptr<MeshObject> MeshManager::LoadMesh(const std::string& filename)
{
    throw std::runtime_error("Not implemented yet");
}

std::shared_ptr<MeshObject> MeshManager::CreateCube(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_cube)
        return _cube;

    static const std::vector<uint32_t> indices =
    {
        //back
        0, 2, 1,
        1, 2, 3,
        //front +4
        4, 5, 7,
        4, 7, 6,
        //left +8
        9, 8, 10,
        10, 8, 11,
        //right +12
        13, 14, 12,
        14, 15, 12,
        //front +16
        17, 16, 19,
        18, 17, 19,
        //back +20
        21, 23, 20,
        22, 23, 21
    };

    _cube = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
        sizeof(geometryVertex),
        indices,
        _device,
        true,
        cmdListExecutor);

    return _cube;
}

std::shared_ptr<MeshObject> MeshManager::CreateEmptyCube(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_emptyCube)
        return _emptyCube;

    static const std::vector<uint32_t> indices =
    {
        //back
        0, 2, 1,
        1, 2, 3,
        24 + 0, 24 + 1, 24 + 2,
        24 + 1, 24 + 3, 24 + 2,
        //front
        4, 5, 7,
        4, 7, 6,
        24 + 4, 24 + 7, 24 + 5,
        24 + 4, 24 + 6, 24 + 7,
        // right
        17, 16, 19,
        18, 17, 19,
        24 + 16, 24 + 17, 24 + 19,
        24 + 17, 24 + 18, 24 + 19,
        // left
        21, 23, 20,
        22, 23, 21,
        24 + 23, 24 + 21, 24 + 20,
        24 + 23, 24 + 22, 24 + 21
    };

    _emptyCube = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
        sizeof(geometryVertex), indices, _device, true, cmdListExecutor);

    return _emptyCube;
}

std::shared_ptr<MeshObject> MeshManager::CreateSphere(std::function<void(CommandList&)> cmdListExecutor)
{
    throw std::runtime_error("Not implemented yet");
}

std::shared_ptr<MeshObject> MeshManager::CreatePlane(std::function<void(CommandList&)> cmdListExecutor)
{
    if (_plane)
        return _plane;

    static const std::vector<geometryVertex> vertices =
    {
        {{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    };

    static const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        0, 2, 3
    };

    _plane = std::make_shared<MeshObject>(
        std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
        sizeof(geometryVertex), indices, _device, true, cmdListExecutor);

    return _plane;
}

std::shared_ptr<MeshObject> MeshManager::CreateScreenQuad()
{
    if (_screenQuad)
        return _screenQuad;

    static std::vector<screenQuadVertex> sqVertices =
    {
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    };

    _screenQuad = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)sqVertices.data(), (uint8_t*)(sqVertices.data() + sqVertices.size())},
                                               sizeof(screenQuadVertex),
                                               std::vector<uint32_t>{},
                                               _device, false);

    return _screenQuad;
}
