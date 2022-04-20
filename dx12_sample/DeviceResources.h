#pragma once

#include "stdafx.h"

class DeviceResources
{
public:
    DeviceResources(ComPtr<ID3D12Device5>      device,
                    ComPtr<IDXGIFactory4>      factory,
                    ComPtr<IDXGISwapChain3>    swapChain,
                    ComPtr<ID3D12CommandQueue> cmdQueue)
        : _device(device)
        , _factory(factory)
        , _swapChain(swapChain)
        , _cmdQueue(cmdQueue)
    {
        assert(device);
        assert(factory);
        assert(swapChain);
        assert(cmdQueue);

        // Have to create Fence and Event.
        ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_frameFence)));
        _frameEndEvent = CreateEvent(NULL, FALSE, FALSE, nullptr);
        assert(_frameEndEvent);
    }

    ComPtr<ID3D12Device5> GetDevice() const
    {
        return _device;
    }

    ComPtr<ID3D12CommandQueue> GetCommandQueue() const
    {
        return _cmdQueue;
    }

    ComPtr<IDXGISwapChain3> GetSwapChain() const
    {
        return _swapChain;
    }

    void WaitForCurrentFrame()
    {
        uint64_t newFenceValue = _fenceValue;

        ThrowIfFailed(_cmdQueue->Signal(_frameFence.Get(), newFenceValue));
        _fenceValue++;

        if (_frameFence->GetCompletedValue() != newFenceValue)
        {
            ThrowIfFailed(_frameFence->SetEventOnCompletion(newFenceValue, _frameEndEvent));
            WaitForSingleObject(_frameEndEvent, INFINITE);
        }
    }

private:
    ComPtr<ID3D12Device5>      _device;
    ComPtr<IDXGIFactory4>      _factory;
    ComPtr<IDXGISwapChain3>    _swapChain;
    ComPtr<ID3D12CommandQueue> _cmdQueue;

    // sync primitives
    HANDLE              _frameEndEvent = nullptr;
    uint64_t            _fenceValue    = 0;
    ComPtr<ID3D12Fence> _frameFence    = nullptr;
};
