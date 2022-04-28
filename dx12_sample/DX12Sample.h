#pragma once

#include "stdafx.h"

#include "DXSample.h"
#include "DeviceResources.h"
#include "SceneManager.h"

#include <utils/SceneObject.h>

struct MouseSceneTracker
{
    bool                     lBtnPressed = false;
    bool                     rBtnPressed = false;
    POINT                    pressedPoint{0, 0};
    POINT                    currPoint{0, 0};
    Graphics::CameraPosition camPosition;
};

class DX12Sample : public DXSample
{
public:
    DX12Sample(int windowWidth, int windowHeight, std::set<optTypes>& opts);
    ~DX12Sample();

    void OnInit() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnDestroy() override;
    bool OnEvent(MSG msg) override;
    void HandleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    void AdjustSizes();

    std::shared_ptr<DeviceResources> CreateDeviceResources();

    ComPtr<IDXGIFactory4>      CreateDXGIFactory();
    ComPtr<ID3D12Device5>      CreateDevice(ComPtr<IDXGIFactory4> factory);
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device> device);
    ComPtr<IDXGISwapChain3>    CreateSwapChain(ComPtr<IDXGIFactory4>      factory,
                                               ComPtr<ID3D12CommandQueue> cmdQueue,
                                               HWND                       hwnd,
                                               size_t                     buffersCount,
                                               unsigned                   width,
                                               unsigned                   height);

    void DumpFeatures();

    void CreateObjects();

    std::shared_ptr<DeviceResources>     _deviceResources = nullptr;
    std::unique_ptr<SceneManager>        _sceneManager    = nullptr;
    std::unique_ptr<RenderTargetManager> _RTManager       = nullptr;
    CommandLineOptions                   _cmdLineOpts{};

    // Main sample parameters
    static constexpr size_t _swapChainBuffersCount = 2;
    bool                    _isResizing            = false;

    ComPtr<ID3D12DescriptorHeap> _uiDescriptors = nullptr;
    std::unique_ptr<CommandList> _uiCmdList;
    std::array<float, 30>        _frameTimes = {};

    MouseSceneTracker _mouseSceneTracker;
};
