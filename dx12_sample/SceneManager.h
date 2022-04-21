#pragma once

#include "stdafx.h"

#include "DeviceResources.h"

#include <utils/CommandList.h>
#include <utils/ComputePipelineState.h>
#include <utils/GraphicsPipelineState.h>
#include <utils/MeshManager.h>
#include <utils/RenderTargetManager.h>
#include <utils/RootSignature.h>
#include <utils/SceneObject.h>
#include <utils/ShaderTable.h>
#include <utils/SphericalCamera.h>
#include <utils/Types.h>

class SceneManager
{
public:
    using SceneObjectPtr = std::shared_ptr<SceneObject>;

    SceneManager(std::shared_ptr<DeviceResources>           deviceResources,
                 UINT                                       screenWidth,
                 UINT                                       screenHeight,
                 CommandLineOptions                         cmdLineOpts,
                 RenderTargetManager*                       rtManager,
                 std::vector<std::shared_ptr<RenderTarget>> swapChainRTs);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;

    void DrawScene();
    void Present();

    // only for texture creation!
    void ExecuteCommandList(const CommandList & commandList);

    Graphics::SphericalCamera& GetCamera();
    void SetLightColor(float r, float g, float b);
    void SetLightPos(float x, float y, float z);
    void SetAmbientColor(float r, float g, float b);

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
    void BuildTLAS();

    // context objects
    std::shared_ptr<DeviceResources>            _deviceResources;

    // command-lists
    std::unique_ptr<CommandList>                _cmdList = nullptr;

    // pipeline states for every object
    ComPtr<ID3D12StateObject>                    _raytracingState = nullptr;

    // root signatures
    RootSignature                               _globalRootSignature;
    RootSignature                               _localRootSignature;

    // shader tables
    std::unique_ptr<ShaderTable>                 _raygenTable;
    std::unique_ptr<ShaderTable>                 _missTable;
    std::unique_ptr<ShaderTable>                 _hitTable;

    // output resources
    ComPtr<ID3D12DescriptorHeap>                _rtsHeap = nullptr;
    ComPtr<ID3D12Resource>                      _raytracingOutput = nullptr;

    // frame resources
    ComPtr<ID3D12Resource>                      _viewParams = nullptr;
    ComPtr<ID3D12Resource>                      _lightParams = nullptr;
    ComPtr<ID3D12Resource>                      _tlas       = nullptr;

    // other objects
    UINT                                        _screenWidth = 0;
    UINT                                        _screenHeight = 0;
    std::vector<std::shared_ptr<RenderTarget>>  _swapChainRTs;
    RenderTargetManager *                       _rtManager = nullptr;
    std::shared_ptr<RenderTarget>               _HDRRt = nullptr;
    Graphics::SphericalCamera                   _mainCamera;
    MeshManager                                 _meshManager;
    std::vector<SceneObjectPtr>                 _sceneObjects;

    float                                       _lightColors[3];
    float                                       _lightPos[3];
    float                                       _ambientColor[3];
};
