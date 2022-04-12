#pragma once

#include "stdafx.h"

#include "DXSample.h"
#include "SceneManager.h"
#include <utils/SceneObject.h>

class DX12Sample :
    public DXSample
{
public:
    DX12Sample(int windowWidth, int windowHeight, std::set<optTypes>& opts);
    ~DX12Sample();

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual bool OnEvent(MSG msg) override;

private:
    void CreateDXGIFactory();
    void CreateDevice();

    void CreateSwapChain();
    void CreateCommandQueue();

    void DumpFeatures();

    std::unique_ptr<SceneManager>               _sceneManager = nullptr;
    std::unique_ptr<RenderTargetManager>        _RTManager = nullptr;

    CommandLineOptions                          _cmdLineOpts {};

    // Main sample parameters
    static constexpr size_t                     _swapChainBuffersCount = 2;

    ComPtr<ID3D12Device5>                       _device = nullptr;
    ComPtr<IDXGIFactory4>                       _DXFactory = nullptr;
    ComPtr<ID3D12CommandQueue>                  _cmdQueue = nullptr;
    ComPtr<IDXGISwapChain3>                     _swapChain = nullptr;
};
