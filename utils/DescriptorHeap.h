#pragma once

#include "stdafx.h"

template<typename T>
struct FreeAddress
{
    std::size_t index;
    T           handle;
};

// This class wraps ID3D12DescriptorHeap and manages allocations
// of individual descriptors. It works similar to std::vector
// but provides address to write to instead of push_back method.
class DescriptorHeap
{
public:
    DescriptorHeap(ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t initialDescriptorsCount);

    // Returns address of descriptor in the heap
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUAddress(std::size_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUAddress(std::size_t index) const;

    // Returns address of a free descriptor in the heap and reallocates heap if needed
    FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> GetFreeCPUAddress();
    FreeAddress<D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeGPUAddress();

    // Returns number of descriptors in the heap
    std::size_t GetNumDescriptors() const;

    // Returns raw resource
    ComPtr<ID3D12DescriptorHeap> GetResource() const;

private:
    void                         Reallocate(std::size_t newSize);
    ComPtr<ID3D12DescriptorHeap> CreateHeap(std::size_t size);

    ComPtr<ID3D12Device>         _device;
    ComPtr<ID3D12DescriptorHeap> _heap;

    std::size_t _capacity;
    std::size_t _size = 0;

    const std::size_t          _incrementSize;
    D3D12_DESCRIPTOR_HEAP_TYPE _type;
};
