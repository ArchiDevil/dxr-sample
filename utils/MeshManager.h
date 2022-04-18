#pragma once

#include "stdafx.h"

#include "CommandList.h"

class MeshObject
{
public:
    MeshObject(const std::vector<uint8_t>&  vertex_data,
               size_t                       stride,
               const std::vector<uint32_t>& index_data,
               ComPtr<ID3D12Device5>             pDevice,
               bool                              createBlas      = true,
               std::function<void(CommandList&)> cmdListExecutor = {});

    MeshObject(MeshObject&& right) noexcept = default;
    MeshObject& operator=(MeshObject&& right) noexcept = default;

    const ComPtr<ID3D12Resource>& VertexBuffer() const;
    const ComPtr<ID3D12Resource>& IndexBuffer() const;
    const ComPtr<ID3D12Resource>& BLAS() const;

    size_t VerticesCount() const;
    size_t IndicesCount() const;

private:
    size_t                      _verticesCount = 0;
    size_t                      _indicesCount = 0;

    ComPtr<ID3D12Resource>      _vertexBuffer = nullptr;
    ComPtr<ID3D12Resource>      _indexBuffer = nullptr;
    ComPtr<ID3D12Resource>      _blas;
};

class MeshManager
{
public:
    MeshManager(ComPtr<ID3D12Device5> device);

    std::shared_ptr<MeshObject> LoadMesh(const std::string& filename);
    std::shared_ptr<MeshObject> CreateCube(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateEmptyCube(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateSphere(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreatePlane(std::function<void(CommandList&)> cmdListExecutor);
    std::shared_ptr<MeshObject> CreateScreenQuad();

private:
    std::vector<std::shared_ptr<MeshObject>> _meshes;
    ComPtr<ID3D12Device5>       _device = nullptr;

    std::shared_ptr<MeshObject> _screenQuad = nullptr;
    std::shared_ptr<MeshObject> _plane = nullptr;
    std::shared_ptr<MeshObject> _cube = nullptr;
    std::shared_ptr<MeshObject> _emptyCube = nullptr;

};
