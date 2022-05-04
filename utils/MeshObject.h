#pragma once

#include "stdafx.h"

#include "CommandList.h"

class MeshObject
{
public:
    MeshObject(std::span<const uint8_t>          vertexData,
               std::size_t                       stride,
               std::span<const uint32_t>         indexData,
               ComPtr<ID3D12Device5>             device,
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
