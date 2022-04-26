#pragma once

#include "stdafx.h"

#include "CommandList.h"

class MeshObject
{
public:
    MeshObject(const std::vector<uint8_t>&       vertex_data,
               size_t                            stride,
               const std::vector<uint32_t>&      index_data,
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
    size_t _verticesCount = 0;
    size_t _indicesCount  = 0;

    ComPtr<ID3D12Resource> _vertexBuffer = nullptr;
    ComPtr<ID3D12Resource> _indexBuffer  = nullptr;
    ComPtr<ID3D12Resource> _blas;
};
