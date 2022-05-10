#pragma once

#include "stdafx.h"

#include "DeviceResources.h"
#include "worldgen/WorldGen.h"

#include <utils/CommandList.h>
#include <utils/ComputePipelineState.h>
#include <utils/DescriptorHeap.h>
#include <utils/GraphicsPipelineState.h>
#include <utils/MeshManager.h>
#include <utils/RenderTargetManager.h>
#include <utils/RootSignature.h>
#include <utils/SceneObject.h>
#include <utils/ShaderTable.h>
#include <utils/SphericalCamera.h>
#include <utils/Types.h>
#include <utils/WASDCamera.h>

class WorldGen;

class SceneManager
{
public:
    using SceneObjectPtr = std::shared_ptr<SceneObject>;
    using SceneObjects   = std::vector<SceneObjectPtr>;

    SceneManager(std::shared_ptr<DeviceResources> deviceResources,
                 UINT                             screenWidth,
                 UINT                             screenHeight,
                 CommandLineOptions               cmdLineOpts,
                 RenderTargetManager*             rtManager);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;

    void DrawScene();
    void Present();

    void ExecuteCommandList(const CommandList& commandList, bool wait = true);

    void SetLightColor(float r, float g, float b);
    void SetLightDirection(float x, float y, float z);
    void SetAmbientColor(float r, float g, float b);

    std::shared_ptr<SceneObject> CreateEmptyCube();
    std::shared_ptr<SceneObject> CreateCube();
    std::shared_ptr<SceneObject> CreateAxis();
    std::shared_ptr<SceneObject> CreateCustomObject(const std::vector<GeometryVertex>& vertices,
                                                    const std::vector<uint32_t>&       indices,
                                                    Material                           material);

    std::shared_ptr<Graphics::SphericalCamera> CreateSphericalCamera();
    std::shared_ptr<Graphics::WASDCamera>      CreateWASDCamera();

    void UpdateWindowSize(UINT screenWidth, UINT screenHeight);

private:
    void CreateRaytracingPSO();
    void CreateRootSignatures();
    void CreateRenderTargets();
    void CreateFrameResources();

    void CreateRayGenMissTables();
    void CreateHitTable();

    void PopulateCommandList();
    void BuildTLAS();

    void PrecomputeTables();

    std::shared_ptr<SceneObject> CreateObject(std::shared_ptr<MeshObject> meshObject, Material material);
    void                         UpdateObjects();
    void                         CheckObjectsState();

    // helper methods
    void CreateConstantBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState);
    void CreateUAVBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState);

    // context objects
    std::shared_ptr<DeviceResources> _deviceResources;

    // command-lists
    CommandList _cmdList;

    // pipeline states for every object
    ComPtr<ID3D12StateObject> _raytracingState = nullptr;

    // root signatures
    RootSignature _globalRootSignature;
    RootSignature _localRootSignature;

    // shader tables
    std::unique_ptr<ShaderTable> _raygenTable;
    std::unique_ptr<ShaderTable> _missTable;
    std::unique_ptr<ShaderTable> _hitTable;

    // output resources
    DescriptorHeap         _descriptorHeap;
    ComPtr<ID3D12Resource> _raytracingOutput = nullptr;

    // renderer resources
    std::size_t _dispatchUavIdx = ~0ULL;
    std::size_t _tlasIdx        = ~0ULL;

    // frame resources
    ComPtr<ID3D12Resource> _viewParams  = nullptr;
    ComPtr<ID3D12Resource> _lightParams = nullptr;
    ComPtr<ID3D12Resource> _tlas        = nullptr;

    // other objects
    UINT                          _screenWidth  = 0;
    UINT                          _screenHeight = 0;
    RenderTargetManager*          _rtManager    = nullptr;
    std::shared_ptr<RenderTarget> _HDRRt        = nullptr;
    MeshManager                   _meshManager;
    SceneObjects                  _sceneObjects;

    std::shared_ptr<Graphics::AbstractCamera> _mainCamera;

    float _lightColors[3];
    float _lightDir[3];
    float _ambientColor[3];

    bool _isObjectsChanged = false;

    // precomputed scattering things
    ComPtr<ID3D12Resource> _inscatterTable;
    ComPtr<ID3D12Resource> _irradianceTable;

};
