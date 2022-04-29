#include "stdafx.h"

#include "RenderTargetManager.h"

RenderTargetManager::RenderTargetManager(ComPtr<ID3D12Device> device)
    : _device(device)
    , _rtvHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64)
    , _dsvHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64)
{
    assert(device);
}

std::shared_ptr<RenderTarget> RenderTargetManager::CreateRenderTarget(DXGI_FORMAT              format,
                                                                      UINT64                   width,
                                                                      UINT                     height,
                                                                      const std::wstring&      rtName /*= L""*/,
                                                                      bool                     isUAV /*= false*/,
                                                                      const D3D12_CLEAR_VALUE* clearValue /*= nullptr*/)
{
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Format              = format;
    resourceDesc.Width               = width;
    resourceDesc.Height              = height;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Alignment           = 0;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    if (!isUAV)
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    else
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE defaultClearValue = {};
    if (!clearValue)
    {
        defaultClearValue.Format   = format;
        defaultClearValue.Color[0] = 0.0f;
        defaultClearValue.Color[1] = 0.0f;
        defaultClearValue.Color[2] = 0.0f;
        defaultClearValue.Color[3] = 1.0f;
    }
    else
    {
        defaultClearValue = *clearValue;
    }

    ComPtr<ID3D12Resource> resource = nullptr;

    ThrowIfFailed(_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET, &defaultClearValue,
                                                   IID_PPV_ARGS(&resource)));

    if (!rtName.empty())
        resource->SetName(rtName.c_str());

    auto freeHandle = _rtvHeap.GetFreeCPUAddress();
    _device->CreateRenderTargetView(resource.Get(), nullptr, freeHandle.handle);
    std::shared_ptr<RenderTarget> outPtr = std::make_shared<RenderTarget>(freeHandle.index, resource, defaultClearValue);
    return outPtr;
}

std::shared_ptr<DepthStencil> RenderTargetManager::CreateDepthStencil(UINT64                   width,
                                                                      UINT                     height,
                                                                      DXGI_FORMAT              format,
                                                                      DXGI_FORMAT              viewFormat,
                                                                      const std::wstring&      dsName /*= L""*/,
                                                                      bool                     isUAV /*= false*/,
                                                                      const D3D12_CLEAR_VALUE* clearValue /*= nullptr*/)
{
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Format              = format;
    resourceDesc.Width               = width;
    resourceDesc.Height              = height;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Alignment           = 0;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    if (!isUAV)
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    else
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE defaultClearValue = {};
    if (!clearValue)
    {
        defaultClearValue.Format               = viewFormat;
        defaultClearValue.DepthStencil.Depth   = 1.0f;
        defaultClearValue.DepthStencil.Stencil = 0;
    }
    else
    {
        defaultClearValue = *clearValue;
    }

    ComPtr<ID3D12Resource> resource = nullptr;

    ThrowIfFailed(_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                   D3D12_RESOURCE_STATE_COMMON, &defaultClearValue, IID_PPV_ARGS(&resource)));

    if (!dsName.empty())
        resource->SetName(dsName.c_str());

    auto freeHandle = _dsvHeap.GetFreeCPUAddress();

    D3D12_DEPTH_STENCIL_VIEW_DESC viewDescription = {};
    viewDescription.Format                        = viewFormat;
    viewDescription.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;

    _device->CreateDepthStencilView(resource.Get(), &viewDescription, freeHandle.handle);
    std::shared_ptr<DepthStencil> outPtr = std::make_shared<DepthStencil>(freeHandle.index, resource, defaultClearValue);
    return outPtr;
}

std::vector<std::shared_ptr<RenderTarget>> RenderTargetManager::CreateRenderTargetsForSwapChain(ComPtr<IDXGISwapChain> swapChain)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    ThrowIfFailed(swapChain->GetDesc(&swapChainDesc));

    std::vector<std::shared_ptr<RenderTarget>> outVector = {};

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format            = swapChainDesc.BufferDesc.Format;
    clearValue.Color[0]          = 0.0f;
    clearValue.Color[1]          = 0.0f;
    clearValue.Color[2]          = 0.0f;
    clearValue.Color[3]          = 1.0f;

    for (UINT i = 0; i < swapChainDesc.BufferCount; ++i)
    {
        ComPtr<ID3D12Resource> resource = nullptr;
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&resource)));
        D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
        viewDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
        viewDesc.ViewDimension                 = D3D12_RTV_DIMENSION_TEXTURE2D;

        auto freeHandle = _rtvHeap.GetFreeCPUAddress();
        _device->CreateRenderTargetView(resource.Get(), &viewDesc, freeHandle.handle);
        resource->SetName(L"SwapChain RT");
        outVector.emplace_back(std::make_shared<RenderTarget>(freeHandle.index, resource, clearValue));
    }

    return outVector;
}

void RenderTargetManager::BindRenderTargets(RenderTarget* renderTargets[8], DepthStencil* depthStencil, CommandList& cmdList)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtCPUDescriptors[8] = {};
    UINT                        descriptorsCount    = 0;
    for (UINT i = 0; i < 8; ++i)
    {
        if (!renderTargets[i])
            break;

        descriptorsCount++;
        rtCPUDescriptors[i] = _rtvHeap.GetCPUAddress(renderTargets[i]->_id);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsCPUDescriptor = {};
    if (depthStencil)
        dsCPUDescriptor = _dsvHeap.GetCPUAddress(depthStencil->_id);

    cmdList->OMSetRenderTargets(descriptorsCount, rtCPUDescriptors, FALSE, depthStencil ? &dsCPUDescriptor : nullptr);
}

void RenderTargetManager::ClearRenderTarget(RenderTarget& renderTarget, CommandList& cmdList)
{
    cmdList->ClearRenderTargetView(_rtvHeap.GetCPUAddress(renderTarget._id), renderTarget._clearValue.Color, 0, nullptr);
}

void RenderTargetManager::ClearDepthStencil(DepthStencil& depthStencil, CommandList& cmdList)
{
    auto flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
    cmdList->ClearDepthStencilView(_dsvHeap.GetCPUAddress(depthStencil._id), flags,
                                   depthStencil._clearValue.DepthStencil.Depth,
                                   depthStencil._clearValue.DepthStencil.Stencil, 0, nullptr);
}
