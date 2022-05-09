#pragma once

#include "stdafx.h"

#include "DXSample.h"
#include "DeviceResources.h"
#include "GameInput.h"
#include "SceneManager.h"
#include "worldgen/WorldGen.h"

#include <utils/SceneObject.h>

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
    void UpdateWorldTexture();
    void CreateUITexture();
    void AdjustSizes();

    void CreateIsland();

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

    ComPtr<ID3D12DescriptorHeap>               _uiDescriptors = nullptr;
    std::unique_ptr<CommandList>               _uiCmdList;
    std::array<float, 30>                      _frameTimes          = {};
    bool                                       _showTerrainControls = false;
    std::shared_ptr<Graphics::WASDCamera>      _camera;

    WorldGen                     _worldGen;
    ComPtr<ID3D12Resource>       _heightMapTexture;
    uint64_t                     _heightMapTexId;

    struct WorldGenParams
    {
        int   octaves     = 6;
        float persistance = 0.5;
        float frequency   = 1.0;
        float lacunarity  = 2.0;
    };

    WorldGenParams             _worldGenParams;

    using ColorsLut = std::map<uint8_t, XMUINT3>;
    ColorsLut _colorsLut = {
        {40, XMUINT3{63, 72, 204}},     // deep water
        {50, XMUINT3{0, 162, 232}},     // shallow water
        {55, XMUINT3{255, 242, 0}},     // sand
        {80, XMUINT3{181, 230, 29}},    // grass
        {95, XMUINT3{34, 177, 76}},     // forest
        {105, XMUINT3{127, 127, 127}},  // rock
        {255, XMUINT3{255, 255, 255}}   // snow
    };
};
