#pragma once

#include "stdafx.h"

struct ShaderEntry
{
    void*       shaderIdentifier = nullptr;
    void*       data             = nullptr;
    std::size_t dataSize         = 0;
};

class ShaderTable
{
public:
    ShaderTable(std::size_t entriesCount, std::size_t dataSize, ComPtr<ID3D12Device> device);

    void AddEntry(std::size_t idx, ShaderEntry entry);

    ComPtr<ID3D12Resource> GetResource() const;
    std::size_t            GetStride() const;
    std::size_t            GetSize() const;

private:
    std::size_t _stride       = 0;
    std::size_t _entriesCount = 0;

    ComPtr<ID3D12Resource> _buffer;
};
