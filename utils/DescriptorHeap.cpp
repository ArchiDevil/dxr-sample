#include "stdafx.h"

#include "DescriptorHeap.h"

DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t initialDescriptorsCount)
    : _capacity(0)
    , _device(device)
    , _incrementSize(device->GetDescriptorHandleIncrementSize(type))
    , _type(type)
{
    if (initialDescriptorsCount == 0)
        initialDescriptorsCount = 32;

    Reallocate(initialDescriptorsCount);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUAddress(std::size_t index) const
{
    auto heapStart = _heap->GetCPUDescriptorHandleForHeapStart();
    heapStart.ptr += _incrementSize * index;
    return heapStart;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUAddress(std::size_t index) const
{
    auto heapStart = _heap->GetGPUDescriptorHandleForHeapStart();
    heapStart.ptr += _incrementSize * index;
    return heapStart;
}

FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> DescriptorHeap::GetFreeCPUAddress()
{
    if (_size == _capacity)
        Reallocate(_size + _size / 3);  // add 33% of the size

    auto heapStart = _heap->GetCPUDescriptorHandleForHeapStart();
    heapStart.ptr += _size * _incrementSize;
    ++_size;
    return {_size - 1, heapStart};
}

FreeAddress<D3D12_GPU_DESCRIPTOR_HANDLE> DescriptorHeap::GetFreeGPUAddress()
{
    if (_size == _capacity)
        Reallocate(_size + _size / 3);  // add 33% of the size

    ++_size;
    auto heapStart = _heap->GetGPUDescriptorHandleForHeapStart();
    heapStart.ptr += _size * _incrementSize;
    return {_size - 1, heapStart};
}

std::size_t DescriptorHeap::GetNumDescriptors() const
{
    return _size;
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::GetResource() const
{
    return _heap;
}

void DescriptorHeap::Reallocate(std::size_t newSize)
{
    assert(newSize > _capacity);
    auto newHeap = CreateHeap(newSize);

    if (_size)
        _device->CopyDescriptorsSimple(_size, newHeap->GetCPUDescriptorHandleForHeapStart(),
                                       _heap->GetCPUDescriptorHandleForHeapStart(), _type);
    _heap     = newHeap;
    _capacity = newSize;
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::CreateHeap(std::size_t size)
{
    ComPtr<ID3D12DescriptorHeap> heap;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors             = static_cast<UINT>(size);
    heapDesc.Type                       = _type;
    heapDesc.Flags = _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV
                         ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                         : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
    return heap;
}
