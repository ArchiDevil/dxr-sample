#pragma once

#include <utils/RenderTargetManager.h>
#include <utils/ComputePipelineState.h>
#include <utils/GraphicsPipelineState.h>
#include <utils/MeshManager.h>
#include <utils/RootSignature.h>
#include <utils/SceneObject.h>
#include <utils/CommandList.h>
#include <utils/Types.h>
#include <utils/SphericalCamera.h>

#include "stdafx.h"

class SceneManager
{
public:
    using SceneObjectPtr = std::shared_ptr<SceneObject>;

    SceneManager(ComPtr<ID3D12Device5> pDevice,
                 UINT screenWidth,
                 UINT screenHeight,
                 CommandLineOptions cmdLineOpts,
                 ComPtr<ID3D12CommandQueue> pCmdQueue,
                 ComPtr<IDXGISwapChain3> pSwapChain,
                 RenderTargetManager * rtManager);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;

    void DrawAll();

    // only for texture creation!
    void ExecuteCommandLists(const CommandList & commandList);

    Graphics::SphericalCamera& GetCamera();

private:
    void CreateCommandLists();
    void CreateConstantBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState);
    void CreateUAVBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState);
    void CreateRaytracingPSO();
    void CreateRootSignatures();
    void CreateRenderTargets();
    void CreateShaderTables();
    void CreateFrameResources();

    void UpdateObjects();
    void PopulateCommandList();

    void WaitCurrentFrame();

    // context objects
    ComPtr<ID3D12Device5>                        _device = nullptr;
    ComPtr<ID3D12CommandQueue>                  _cmdQueue = nullptr;
    ComPtr<IDXGISwapChain3>                     _swapChain = nullptr;

    // command-lists
    std::unique_ptr<CommandList>                _cmdList = nullptr;

    // pipeline states for every object
    ComPtr<ID3D12StateObject>                    _raytracingState = nullptr;

    // sync primitives
    HANDLE                                      _frameEndEvent = nullptr;
    uint64_t                                    _fenceValue = 0;
    uint32_t                                    _frameIndex = 0;
    ComPtr<ID3D12Fence>                         _frameFence = nullptr;
    bool                                        _isFrameWaiting = false;

    // root signatures
    RootSignature                               _globalRootSignature;

    // shader tables
    ComPtr<ID3D12Resource>                      _raygenTable = nullptr;
    ComPtr<ID3D12Resource>                      _missTable = nullptr;

    // output resources
    ComPtr<ID3D12DescriptorHeap>                _rtsHeap = nullptr;
    ComPtr<ID3D12Resource>                      _raytracingOutput = nullptr;

    // frame resources
    ComPtr<ID3D12Resource>                      _viewParams = nullptr;
    ComPtr<ID3D12Resource>                      _tlas       = nullptr;

    // other objects from outside
    UINT                                        _screenWidth = 0;
    UINT                                        _screenHeight = 0;
    std::vector<std::shared_ptr<RenderTarget>>  _swapChainRTs;
    RenderTargetManager *                       _rtManager = nullptr;
    std::shared_ptr<RenderTarget>               _HDRRt = nullptr;
    Graphics::SphericalCamera                   _mainCamera;
    MeshManager                                 _meshManager;
    std::vector<SceneObjectPtr>                 _sceneObjects;
    void                                        BuildTLAS();
};
