#include "stdafx.h"

#include "ShaderTable.h"

#include "Math.h"

////////////////////////////////////////////////////////////////////////////////

ShaderTable::ShaderTable(std::size_t entriesCount, std::size_t dataSize, ComPtr<ID3D12Device> device)
    : _entriesCount(entriesCount)
{
    // dataSize may be null if no local data is used
    assert(entriesCount > 0);

    _stride                         = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + dataSize;
    _stride                         = Math::AlignTo(_stride, 32);
    D3D12_HEAP_PROPERTIES heapProps = {D3D12_HEAP_TYPE_UPLOAD};

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width               = _entriesCount * _stride;
    bufferDesc.Height              = 1;
    bufferDesc.MipLevels           = 1;
    bufferDesc.SampleDesc.Count    = 1;
    bufferDesc.DepthOrArraySize    = 1;
    bufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_buffer)));
}

void ShaderTable::AddEntry(std::size_t idx, ShaderEntry entry)
{
    assert(idx < _entriesCount);
    assert(entry.shaderIdentifier);

    // check if we do not overrun allocated memory
    assert(idx * _stride <= GetSize());

    void* data = nullptr;
    _buffer->Map(0, nullptr, &reinterpret_cast<void*>(data));

    uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
    ptr += _stride * idx;

    std::memcpy(ptr, entry.shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    ptr += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    std::memcpy(ptr, entry.data, entry.dataSize);

    _buffer->Unmap(0, nullptr);
}

ComPtr<ID3D12Resource> ShaderTable::GetResource() const
{
    return _buffer;
}

std::size_t ShaderTable::GetStride() const
{
    return _stride;
}

std::size_t ShaderTable::GetSize() const
{
    return _entriesCount * _stride;
}
