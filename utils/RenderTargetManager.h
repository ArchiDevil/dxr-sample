#pragma once

#include "stdafx.h"

#include "CommandList.h"
#include "DescriptorHeap.h"

struct RenderTarget
{
    RenderTarget(uint64_t id, ComPtr<ID3D12Resource> resource, D3D12_CLEAR_VALUE clearValue)
        : _id(id)
        , _texture(resource)
        , _clearValue(clearValue)
    {
    }

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget& operator=(RenderTarget&&) = delete;

    const uint64_t                  _id;
    const ComPtr<ID3D12Resource>    _texture;
    const D3D12_CLEAR_VALUE         _clearValue;
};

struct DepthStencil
{
    DepthStencil(uint64_t id, ComPtr<ID3D12Resource> resource, D3D12_CLEAR_VALUE clearValue)
        : _id(id)
        , _texture(resource)
        , _clearValue(clearValue)
    {
    }

    DepthStencil(const DepthStencil&) = delete;
    DepthStencil(DepthStencil&&) = delete;
    DepthStencil& operator=(const DepthStencil&) = delete;
    DepthStencil& operator=(DepthStencil&&) = delete;

    const uint64_t                  _id;
    const ComPtr<ID3D12Resource>    _texture;
    const D3D12_CLEAR_VALUE         _clearValue;
};

class RenderTargetManager
{
public:
    RenderTargetManager(ComPtr<ID3D12Device> device);

    std::shared_ptr<RenderTarget> CreateRenderTarget(DXGI_FORMAT format,
                                                     UINT64 width,
                                                     UINT height,
                                                     const std::wstring& rtName = L"",
                                                     bool isUAV = false,
                                                     const D3D12_CLEAR_VALUE* clearValue = nullptr);

    std::shared_ptr<DepthStencil> CreateDepthStencil(UINT64 width,
                                                     UINT height, 
                                                     DXGI_FORMAT format,
                                                     DXGI_FORMAT viewFormat,
                                                     const std::wstring& dsName = L"",
                                                     bool isUAV = false,
                                                     const D3D12_CLEAR_VALUE* clearValue = nullptr);

    std::vector<std::shared_ptr<RenderTarget>> CreateRenderTargetsForSwapChain(ComPtr<IDXGISwapChain> swapChain);

    void BindRenderTargets(RenderTarget* renderTargets[8],
                           DepthStencil* depthStencil,
                           CommandList & cmdList);

    void ClearRenderTarget(RenderTarget& renderTarget,
                           CommandList & cmdList);

    void ClearDepthStencil(DepthStencil& depthStencil,
                           CommandList & cmdList);

private:
    DescriptorHeap       _rtvHeap;
    DescriptorHeap       _dsvHeap;
    ComPtr<ID3D12Device> _device = nullptr;

};
