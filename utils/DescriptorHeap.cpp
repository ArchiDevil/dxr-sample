#include "stdafx.h"

#include "DescriptorHeap.h"

DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t initialDescriptorsCount)
    : _capacity(0)
    , _device(device)
    , _incrementSize(device->GetDescriptorHandleIncrementSize(type))
    , _type(type)
{
    if (!initialDescriptorsCount)
        initialDescriptorsCount = 32;

    Reallocate(initialDescriptorsCount);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUAddress(std::size_t index) const
{
    auto heapStart = _cpuHeap->GetCPUDescriptorHandleForHeapStart();
    heapStart.ptr += _incrementSize * index;
    return heapStart;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUAddress(std::size_t index) const
{
    auto heapStart = _gpuHeap->GetGPUDescriptorHandleForHeapStart();
    heapStart.ptr += _incrementSize * index;
    return heapStart;
}

FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> DescriptorHeap::GetFreeCPUAddress()
{
    _dirty = true;

    if (_size == _capacity)
        Reallocate(_size + _size / 3);  // add 33% of the size

    auto heapStart = _cpuHeap->GetCPUDescriptorHandleForHeapStart();
    heapStart.ptr += _size * _incrementSize;
    ++_size;
    return {_size - 1, heapStart};
}

std::size_t DescriptorHeap::GetNumDescriptors() const
{
    return _size;
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::GetResource()
{
    if (IsDirty())
    {
        // Mirror descriptors
        Mirror();
    }

    return _gpuHeap;
}

bool DescriptorHeap::IsDirty() const
{
    return _dirty;
}

void DescriptorHeap::Mirror()
{
    if (_size)
    {
        _device->CopyDescriptorsSimple(_size, _gpuHeap->GetCPUDescriptorHandleForHeapStart(),
                                       _cpuHeap->GetCPUDescriptorHandleForHeapStart(), _type);
    }
}

void DescriptorHeap::Reallocate(std::size_t newSize)
{
    assert(newSize > _capacity);
    auto newHeap = CreateHeap(newSize, false);

    if (_size)
        _device->CopyDescriptorsSimple(_size, newHeap->GetCPUDescriptorHandleForHeapStart(),
                                       _cpuHeap->GetCPUDescriptorHandleForHeapStart(), _type);

    const bool visible = _type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV ? false : true;

    _cpuHeap  = newHeap;
    _gpuHeap  = CreateHeap(newSize, visible);
    _capacity = newSize;
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::CreateHeap(std::size_t size, bool visible)
{
    ComPtr<ID3D12DescriptorHeap> heap;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors             = static_cast<UINT>(size);
    heapDesc.Type                       = _type;
    if (visible)
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
    return heap;
}
