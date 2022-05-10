#include "stdafx.h"

#include "SceneManager.h"

#include <shaders/Common.h>
#include <utils/Filesystem.h>
#include <utils/RenderTargetManager.h>

#include <filesystem>
#include <iostream>
#include <random>

constexpr auto pi = 3.14159265f;

SceneManager::SceneManager(std::shared_ptr<DeviceResources> deviceResources,
                           UINT                             screenWidth,
                           UINT                             screenHeight,
                           CommandLineOptions               cmdLineOpts,
                           RenderTargetManager*             rtManager)
    : _deviceResources(deviceResources)
    , _rtManager(rtManager)
    , _meshManager(_deviceResources->GetDevice())
    , _descriptorHeap(_deviceResources->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512)
    , _cmdList(CommandListType::Direct, _deviceResources->GetDevice())
{
    assert(rtManager);

    _cmdList.Close();

    SetThreadDescription(GetCurrentThread(), L"Main thread");

    PrecomputeTables();

    CreateFrameResources();
    UpdateWindowSize(screenWidth, screenHeight);
    CreateRootSignatures();
    CreateRaytracingPSO();

    CreateRayGenMissTables();
}

SceneManager::~SceneManager()
{
    _deviceResources->WaitForCurrentFrame();
}

void SceneManager::DrawScene()
{
    CheckObjectsState();

    std::vector<ID3D12CommandList*> cmdListArray;
    cmdListArray.reserve(16); // just to avoid any allocations
    UpdateObjects();
    PopulateCommandList();
    cmdListArray.push_back(_cmdList.GetInternal().Get());

    _deviceResources->GetCommandQueue()->ExecuteCommandLists(1, cmdListArray.data());
}

void SceneManager::Present()
{
    // Swap buffers
    _deviceResources->GetSwapChain()->Present(0, 0);
    _deviceResources->WaitForCurrentFrame();
}

void SceneManager::ExecuteCommandList(const CommandList& commandList, bool wait /*= true*/)
{
    std::array<ID3D12CommandList*, 1> cmdListsArray = {commandList.GetInternal().Get()};
    _deviceResources->GetCommandQueue()->ExecuteCommandLists((UINT)cmdListsArray.size(), cmdListsArray.data());

    if (wait)
        _deviceResources->WaitForCurrentFrame();
}

void SceneManager::SetLightColor(float r, float g, float b)
{
    _lightColors[0] = r;
    _lightColors[1] = g;
    _lightColors[2] = b;
}

void SceneManager::SetLightDirection(float x, float y, float z)
{
    _lightDir[0] = x;
    _lightDir[1] = y;
    _lightDir[2] = z;
}

void SceneManager::SetAmbientColor(float r, float g, float b)
{
    _ambientColor[0] = r;
    _ambientColor[1] = g;
    _ambientColor[2] = b;
}

void SceneManager::UpdateWindowSize(UINT screenWidth, UINT screenHeight)
{
    if (_screenHeight == screenHeight && _screenWidth == screenWidth)
        return;

    _screenWidth  = screenWidth;
    _screenHeight = screenHeight;

    CreateRenderTargets();
    if (_mainCamera)
        _mainCamera->SetScreenParams((float)_screenWidth, (float)_screenHeight);
}

void SceneManager::CreateRenderTargets()
{
    D3D12_RESOURCE_DESC dxrOutputDesc = {};
    dxrOutputDesc.Width               = _screenWidth;
    dxrOutputDesc.Height              = _screenHeight;
    dxrOutputDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dxrOutputDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxrOutputDesc.MipLevels           = 1;
    dxrOutputDesc.DepthOrArraySize    = 1;
    dxrOutputDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    dxrOutputDesc.SampleDesc.Count    = 1;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    ThrowIfFailed(_deviceResources->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &dxrOutputDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&_raytracingOutput)));

    FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> heapHandle = {};
    if (_dispatchUavIdx == ~0ULL)
    {
        heapHandle = _descriptorHeap.GetFreeCPUAddress();
        _dispatchUavIdx = heapHandle.index;
    }
    else
    {
        heapHandle.index = _dispatchUavIdx;
        heapHandle.handle = _descriptorHeap.GetCPUAddress(_dispatchUavIdx);
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    _deviceResources->GetDevice()->CreateUnorderedAccessView(_raytracingOutput.Get(), nullptr, &uavDesc, heapHandle.handle);
}

void SceneManager::CreateRayGenMissTables()
{
    _raygenTable = std::make_unique<ShaderTable>(1, 0, _deviceResources->GetDevice());
    _missTable   = std::make_unique<ShaderTable>(1, 0, _deviceResources->GetDevice());

    ComPtr<ID3D12StateObjectProperties> props;
    ThrowIfFailed(_raytracingState->QueryInterface(props.GetAddressOf()));

    _raygenTable->AddEntry(0, ShaderEntry{props->GetShaderIdentifier(L"RayGenShader")});
    _missTable->AddEntry(0, ShaderEntry{props->GetShaderIdentifier(L"MissShader")});
}

void SceneManager::CreateHitTable()
{
    constexpr uint32_t lrsBindingsCount = 2;

    _hitTable = std::make_unique<ShaderTable>(_sceneObjects.size(), lrsBindingsCount * sizeof(D3D12_GPU_VIRTUAL_ADDRESS),
                                              _deviceResources->GetDevice());

    ComPtr<ID3D12StateObjectProperties> props;
    ThrowIfFailed(_raytracingState->QueryInterface(props.GetAddressOf()));

    // fill shader tables with scene objects data
    for (std::size_t i = 0; i < _sceneObjects.size(); i++)
    {
        auto& object = _sceneObjects[i];

        std::vector<uint64_t> data;
        data.reserve(lrsBindingsCount);

        auto buffersHandle = _descriptorHeap.GetGPUAddress(object->GetDescriptorIdx());
        data.push_back(object->GetConstantBuffer()->GetGPUVirtualAddress());
        data.push_back(buffersHandle.ptr);

        void* shaderIdentifier = nullptr;
        switch (object->GetMaterial().GetType())
        {
        case MaterialType::Diffuse:
            shaderIdentifier = props->GetShaderIdentifier(L"DiffuseHitGroup");
            break;
        case MaterialType::Specular:
            shaderIdentifier = props->GetShaderIdentifier(L"SpecularHitGroup");
            break;
        case MaterialType::Water:
            shaderIdentifier = props->GetShaderIdentifier(L"WaterHitGroup");
            break;
        }

        _hitTable->AddEntry(i, ShaderEntry{shaderIdentifier, data.data(), data.size() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)});
    }
}

void SceneManager::PopulateCommandList()
{
    ComPtr<ID3D12StateObjectProperties> props;
    ThrowIfFailed(_raytracingState->QueryInterface(props.GetAddressOf()));

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.Height = _screenHeight;
    desc.Width = _screenWidth;
    desc.Depth = 1;

    desc.RayGenerationShaderRecord.StartAddress = _raygenTable->GetResource()->GetGPUVirtualAddress();
    desc.RayGenerationShaderRecord.SizeInBytes  = _raygenTable->GetSize();

    desc.MissShaderTable.StartAddress  = _missTable->GetResource()->GetGPUVirtualAddress();
    desc.MissShaderTable.SizeInBytes   = _missTable->GetSize();
    desc.MissShaderTable.StrideInBytes = _missTable->GetStride();

    desc.HitGroupTable.StartAddress  = _hitTable->GetResource()->GetGPUVirtualAddress();
    desc.HitGroupTable.SizeInBytes   = _hitTable->GetSize();
    desc.HitGroupTable.StrideInBytes = _hitTable->GetStride();

    _cmdList.Reset();
    _cmdList->SetPipelineState1(_raytracingState.Get());

    ID3D12DescriptorHeap* heaps[] = { _descriptorHeap.GetResource().Get()};
    _cmdList->SetDescriptorHeaps(1, heaps);
    _cmdList->SetComputeRootSignature(_globalRootSignature.GetInternal().Get());
    _cmdList->SetComputeRootDescriptorTable(0, _descriptorHeap.GetGPUAddress(_dispatchUavIdx));
    _cmdList->SetComputeRootDescriptorTable(1, _descriptorHeap.GetGPUAddress(_tlasIdx));
    _cmdList->SetComputeRootConstantBufferView(2, _viewParams->GetGPUVirtualAddress());
    _cmdList->SetComputeRootConstantBufferView(3, _lightParams->GetGPUVirtualAddress());
    _cmdList->DispatchRays(&desc);

    size_t frameIndex = _deviceResources->GetSwapChain()->GetCurrentBackBufferIndex();
    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_deviceResources->GetSwapChainRts()[frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        _cmdList->ResourceBarrier(1, &transition);
    }

    auto rtBuffer = _deviceResources->GetSwapChainRts()[frameIndex].get();
    _rtManager->ClearRenderTarget(*rtBuffer, _cmdList);

    {
        std::vector< CD3DX12_RESOURCE_BARRIER> transitions(2);
        transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(_deviceResources->GetSwapChainRts()[frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        _cmdList->ResourceBarrier(transitions.size(), transitions.data());
    }

    _cmdList->CopyResource(rtBuffer->_texture.Get(), _raytracingOutput.Get());

    // Indicate that the back buffer will be used as a render target.
    {
        std::vector< CD3DX12_RESOURCE_BARRIER> transitions(2);
        transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(_deviceResources->GetSwapChainRts()[frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        _cmdList->ResourceBarrier(transitions.size(), transitions.data());
    }

    _cmdList->Close();
}

void SceneManager::CreateConstantBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState)
{
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width               = bufferSize;
    constantBufferDesc.Height              = 1;
    constantBufferDesc.MipLevels           = 1;
    constantBufferDesc.SampleDesc.Count    = 1;
    constantBufferDesc.DepthOrArraySize    = 1;
    constantBufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};
    ThrowIfFailed(_deviceResources->GetDevice()->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, initialState, nullptr, IID_PPV_ARGS(&(*pOutBuffer))));
}

void SceneManager::CreateUAVBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer, D3D12_RESOURCE_STATES initialState)
{
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width               = bufferSize;
    bufferDesc.Height              = 1;
    bufferDesc.MipLevels           = 1;
    bufferDesc.SampleDesc.Count    = 1;
    bufferDesc.DepthOrArraySize    = 1;
    bufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_DEFAULT};
    ThrowIfFailed(_deviceResources->GetDevice()->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialState, nullptr, IID_PPV_ARGS(&(*pOutBuffer))));
}

void SceneManager::CreateRaytracingPSO()
{
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;

    D3D12_DXIL_LIBRARY_DESC library_desc = {};

    D3D12_SHADER_BYTECODE bytecode = {};
    std::vector<char> bytes = ReadFileContent("Sample.cso");

    library_desc.DXILLibrary.pShaderBytecode = bytes.data();
    library_desc.DXILLibrary.BytecodeLength = bytes.size();

    std::vector<D3D12_EXPORT_DESC> exports;
    exports.push_back(D3D12_EXPORT_DESC{ L"RayGenShader" });
    exports.push_back(D3D12_EXPORT_DESC{ L"MissShader" });
    exports.push_back(D3D12_EXPORT_DESC{ L"DiffuseShader" });
    exports.push_back(D3D12_EXPORT_DESC{ L"SpecularShader" });
    exports.push_back(D3D12_EXPORT_DESC{ L"WaterShader" });

    library_desc.NumExports = exports.size();
    library_desc.pExports = exports.data();
    {
        // library
        D3D12_STATE_SUBOBJECT library = {};
        library.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;

        library.pDesc = &library_desc;
        subobjects.emplace_back(library);
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc1   = {};
    hitGroupDesc1.Type = D3D12_HIT_GROUP_TYPE::D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc1.HitGroupExport = L"DiffuseHitGroup";
    hitGroupDesc1.ClosestHitShaderImport = L"DiffuseShader";
    {
        D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
        hitGroupSubobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroupSubobject.pDesc = &hitGroupDesc1;
        subobjects.emplace_back(hitGroupSubobject);
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc2 = {};
    hitGroupDesc2.Type = D3D12_HIT_GROUP_TYPE::D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc2.HitGroupExport = L"SpecularHitGroup";
    hitGroupDesc2.ClosestHitShaderImport = L"SpecularShader";
    {
        D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
        hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroupSubobject.pDesc = &hitGroupDesc2;
        subobjects.emplace_back(hitGroupSubobject);
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc3 = {};
    hitGroupDesc3.Type = D3D12_HIT_GROUP_TYPE::D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc3.HitGroupExport = L"WaterHitGroup";
    hitGroupDesc3.ClosestHitShaderImport = L"WaterShader";
    {
        D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
        hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroupSubobject.pDesc = &hitGroupDesc3;
        subobjects.emplace_back(hitGroupSubobject);
    }

    D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig = {};
    descPipelineConfig.MaxTraceRecursionDepth           = 2;  // TODO(DB): we need to update it when we implement water
    {
        D3D12_STATE_SUBOBJECT pipelineSubobject = {};
        pipelineSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        pipelineSubobject.pDesc = &descPipelineConfig;
        subobjects.emplace_back(pipelineSubobject);
    }

    D3D12_RAYTRACING_SHADER_CONFIG descShaderConfig = {};
    descShaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 4; // TODO(DB): is it enough?
    descShaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
    {
        D3D12_STATE_SUBOBJECT shaderSubobject = {};
        shaderSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        shaderSubobject.pDesc = &descShaderConfig;
        subobjects.emplace_back(shaderSubobject);
    }

    D3D12_GLOBAL_ROOT_SIGNATURE grsDesc = {};
    grsDesc.pGlobalRootSignature = _globalRootSignature.GetInternal().Get();
    {
        D3D12_STATE_SUBOBJECT globalRootSignature = {};
        globalRootSignature.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        globalRootSignature.pDesc = &grsDesc;
        subobjects.emplace_back(globalRootSignature);
    }

    D3D12_LOCAL_ROOT_SIGNATURE lrsDesc = {};
    lrsDesc.pLocalRootSignature = _localRootSignature.GetInternal().Get();
    {
        D3D12_STATE_SUBOBJECT localRootSignature = {};
        localRootSignature.Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        localRootSignature.pDesc = &lrsDesc;
        subobjects.emplace_back(localRootSignature);
    }

    D3D12_STATE_OBJECT_DESC pDesc = {};
    pDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    pDesc.NumSubobjects = subobjects.size();
    pDesc.pSubobjects = subobjects.data();

    ThrowIfFailed(_deviceResources->GetDevice()->CreateStateObject(&pDesc, IID_PPV_ARGS(&_raytracingState)));
}

void SceneManager::CreateRootSignatures()
{
    _globalRootSignature.Init(4, 0);
    _globalRootSignature[0].InitAsDescriptorsTable(1); // Output UAV
    _globalRootSignature[0].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);

    _globalRootSignature[1].InitAsDescriptorsTable(1); // TLAS
    _globalRootSignature[1].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _globalRootSignature[2].InitAsCBV(0);
    _globalRootSignature[3].InitAsCBV(1);
    _globalRootSignature.Finalize(_deviceResources->GetDevice());

    _localRootSignature.Init(2, 0);
    _localRootSignature[0].InitAsCBV(2);
    _localRootSignature[1].InitAsDescriptorsTable(1); // VB/IB views
    _localRootSignature[1].InitTableRange(0, 1, 2, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    _localRootSignature.Finalize(_deviceResources->GetDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void SceneManager::CreateFrameResources()
{
    CreateConstantBuffer(sizeof(ViewParams), &_viewParams, D3D12_RESOURCE_STATE_GENERIC_READ);
    CreateConstantBuffer(sizeof(LightParams), &_lightParams, D3D12_RESOURCE_STATE_GENERIC_READ);
}

std::shared_ptr<SceneObject> SceneManager::CreateObject(std::shared_ptr<MeshObject> meshObject, Material material)
{
    _isObjectsChanged = true;

    return _sceneObjects.emplace_back(
        std::make_shared<SceneObject>(
            meshObject,
            _descriptorHeap,
            _deviceResources->GetDevice(),
            material
            )
    );
}

std::shared_ptr<SceneObject> SceneManager::CreateEmptyCube()
{
    return CreateObject(
        _meshManager.CreateEmptyCube([this](CommandList& cmdList) { ExecuteCommandList(cmdList); }),
        Material{MaterialType::Diffuse}
    );
}

std::shared_ptr<SceneObject> SceneManager::CreateCube()
{
    return CreateObject(
        _meshManager.CreateCube([this](CommandList& cmdList) { ExecuteCommandList(cmdList); }),
        Material(MaterialType::Specular)
    );
}

std::shared_ptr<SceneObject> SceneManager::CreateAxis()
{
    return CreateObject(_meshManager.CreateAxes([this](CommandList& cmdList) { ExecuteCommandList(cmdList); }),
                        Material{MaterialType::Diffuse});
}

std::shared_ptr<SceneObject> SceneManager::CreateCustomObject(const std::vector<GeometryVertex>& vertices,
                                                              const std::vector<uint32_t>&       indices,
                                                              Material                           material)
{
    return CreateObject(
        _meshManager.CreateCustomObject(
            vertices,
            indices,
            [this](CommandList& cmdList) { ExecuteCommandList(cmdList); }
        ),
        material
    );
}

std::shared_ptr<Graphics::SphericalCamera> SceneManager::CreateSphericalCamera()
{
    const float znear = 0.1f;
    const float zfar = 3100.f;
    const float fov = 7.5f * pi / 18.0f;

    auto output = std::make_shared<Graphics::SphericalCamera>(znear, zfar, fov, (float)_screenWidth, (float)_screenHeight);

    output->SetRadius(10.0f);
    output->SetRotation(45.0f);
    output->SetInclination(20.0f);

    _mainCamera = output;
    return output;
}

std::shared_ptr<Graphics::WASDCamera> SceneManager::CreateWASDCamera()
{
    const float znear = 0.1f;
    const float zfar  = 3100.f;
    const float fov = 7.5f * pi / 18.0f;

    auto output = std::make_shared<Graphics::WASDCamera>(znear, zfar, fov, (float)_screenWidth, (float)_screenHeight);

    _mainCamera = output;
    return output;
}

void SceneManager::UpdateObjects()
{
    void* sceneData = nullptr;

    ThrowIfFailed(_viewParams->Map(0, nullptr, &sceneData));

    DirectX::XMFLOAT3 camPos = _mainCamera ? _mainCamera->GetPosition() : XMFLOAT3{};
    DirectX::XMVECTOR m      = {camPos.x, camPos.y, camPos.z, 1.0f};

    DirectX::XMMATRIX viewProjMat = _mainCamera ? _mainCamera->GetViewProjMatrix() : XMMatrixIdentity();

    ((ViewParams*)sceneData)->viewPos         = m;
    ((ViewParams*)sceneData)->inverseViewProj = DirectX::XMMatrixInverse(nullptr, viewProjMat);
    ((ViewParams*)sceneData)->ambientColor    = DirectX::XMVECTOR({ _ambientColor[0], _ambientColor[1], _ambientColor[2], 1.0 });

    ThrowIfFailed(_lightParams->Map(0, nullptr, &sceneData));

    ((LightParams*)sceneData)->direction = DirectX::XMVECTOR({ _lightDir[0], _lightDir[1], _lightDir[2], 1.0});
    ((LightParams*)sceneData)->color     = DirectX::XMVECTOR({_lightColors[0], _lightColors[1], _lightColors[2], 1.0});

    bool anyDirty = std::ranges::any_of(_sceneObjects.cbegin(), _sceneObjects.cend(),
        [](const SceneObjectPtr& object) { return object->IsDirty(); });

    if (anyDirty)
    {
        BuildTLAS();
        std::ranges::for_each(_sceneObjects, [](SceneObjectPtr& object) { object->ResetDirty(); });
    }
}

void SceneManager::CheckObjectsState()
{
    if (!_isObjectsChanged)
        return;

    CreateHitTable();
    _isObjectsChanged = false;
}

void SceneManager::BuildTLAS()
{
    CommandList cmdList{CommandListType::Direct, _deviceResources->GetDevice() };

    size_t objectsCount = _sceneObjects.size();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
    auto& inputs = tlasDesc.Inputs;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE; // TODO (DB): check what flags we have and how we could use it
    inputs.NumDescs = objectsCount;
    inputs.pGeometryDescs = nullptr;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    _deviceResources->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
        throw std::runtime_error("Zeroed size");

    ComPtr<ID3D12Resource> scratchResource;
    CreateUAVBuffer(prebuildInfo.ScratchDataSizeInBytes, &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    CreateUAVBuffer(prebuildInfo.ResultDataMaxSizeInBytes, &_tlas, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
    _tlas->SetName(L"Top-Level Acceleration Structure");

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instancesDesc;
    instancesDesc.reserve(objectsCount);
    for (std::size_t idx = 0; idx < _sceneObjects.size(); idx++)
    {
        instancesDesc.emplace_back();

        D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc     = instancesDesc.back();
        instanceDesc.AccelerationStructure               = _sceneObjects[idx]->GetMeshObject().BLAS()->GetGPUVirtualAddress();
        instanceDesc.InstanceContributionToHitGroupIndex = idx;
        instanceDesc.InstanceMask                        = 1;

        for (int i = 0; i < 3; ++i)
            memcpy(instanceDesc.Transform[i], &(_sceneObjects[idx]->GetWorldMatrix().r[i]), sizeof(float) * 4);
    }

    ComPtr<ID3D12Resource> instances;
    size_t instancesBufferSize = sizeof(decltype(instancesDesc)::value_type) * instancesDesc.size();
    CreateConstantBuffer(instancesBufferSize, &instances, D3D12_RESOURCE_STATE_GENERIC_READ);
    void* instancesPtr = nullptr;
    instances->Map(0, nullptr, &instancesPtr);
    memcpy(instancesPtr, instancesDesc.data(), instancesBufferSize);
    instances->Unmap(0, nullptr);

    {
        tlasDesc.Inputs.InstanceDescs             = instances->GetGPUVirtualAddress();
        tlasDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        tlasDesc.DestAccelerationStructureData    = _tlas->GetGPUVirtualAddress();
    }

    cmdList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    cmdList.Close();

    ExecuteCommandList(cmdList);

    D3D12_CPU_DESCRIPTOR_HANDLE heapHandle;
    if (_tlasIdx == ~0ULL)
    {
        auto address = _descriptorHeap.GetFreeCPUAddress();
        _tlasIdx = address.index;
        heapHandle = address.handle;
    }
    else
    {
        heapHandle = _descriptorHeap.GetCPUAddress(_tlasIdx);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.RaytracingAccelerationStructure.Location = _tlas->GetGPUVirtualAddress();
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    _deviceResources->GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, heapHandle);
}

void SceneManager::PrecomputeTables()
{
    auto        device = _deviceResources->GetDevice();
    DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    const UINT inscatterMapWidth  = 256;
    const UINT inscatterMapHeight = 128;
    const UINT inscatterMapDepth  = 32;

    const UINT irradianceMapWidth  = 64 * 4;
    const UINT irradianceMapHeight = 16 * 4;

    const int maxScatteringOrder = 4;

    ComPtr<ID3D12PipelineState> copyDeltaEPso;
    RootSignature               copyTextureRS;

    // Temporary buffers (could be removed and put on a stack in PrecomputeTables()
    ComPtr<ID3D12Resource> transmittance;
    ComPtr<ID3D12Resource> deltaE;
    ComPtr<ID3D12Resource> deltaSR; // Rayleigh scattering table
    ComPtr<ID3D12Resource> deltaSM; // Mie scattering table
    ComPtr<ID3D12Resource> deltaJ;  // Sphere scattering table

    // Transmittance computing
    {
        PIXScopedEvent(_deviceResources->GetCommandQueue().Get(), 0, "Transmittance");

        const UINT mapWidth  = 256;
        const UINT mapHeight = 64;

        RootSignature rs;
        rs.Init(1, 0);
        rs[0].InitAsDescriptorsTable(1);
        rs[0].InitTableRange(0, 0, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
        rs.Finalize(device);

        auto                  content = ReadFileContent("TransmittanceCompute.cso");
        D3D12_SHADER_BYTECODE bc      = {};
        bc.BytecodeLength             = content.size();
        bc.pShaderBytecode            = content.data();

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.CS                                = bc;
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.pRootSignature                    = rs.GetInternal().Get();

        ComPtr<ID3D12PipelineState> pso;
        ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));

        DescriptorHeap tempHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Width               = mapWidth;
        texDesc.Height              = mapHeight;
        texDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Format              = format;
        texDesc.MipLevels           = 1;
        texDesc.DepthOrArraySize    = 1;
        texDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        texDesc.SampleDesc.Count    = 1;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;

        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                                      IID_PPV_ARGS(&transmittance)));

        FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> heapHandle = tempHeap.GetFreeCPUAddress();

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format                           = format;
        uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(transmittance.Get(), nullptr, &uavDesc, heapHandle.handle);

        heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(deltaE.Get(), nullptr, &uavDesc, heapHandle.handle);

        CommandList cmdList{CommandListType::Direct, device, pso};

        auto heap = tempHeap.GetResource().Get();
        cmdList->SetDescriptorHeaps(1, &heap);
        cmdList->SetComputeRootSignature(rs.GetInternal().Get());
        cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(0));
        cmdList->Dispatch(mapWidth, mapHeight, 1);
        cmdList.Close();

        ExecuteCommandList(cmdList);
    }

    // Single inscatter computing (delta SR, SM tables)
    {
        PIXScopedEvent(_deviceResources->GetCommandQueue().Get(), 0, "Single inscatter (delta SR, SM)");

        auto                  content = ReadFileContent("SingleInscatterCompute.cso");
        D3D12_SHADER_BYTECODE bc      = {};
        bc.BytecodeLength             = content.size();
        bc.pShaderBytecode            = content.data();

        RootSignature rs;
        rs.Init(1, 0);
        rs[0].InitAsDescriptorsTable(1);
        rs[0].InitTableRange(0, 0, 4, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
        rs.Finalize(device);

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.CS                                = bc;
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.pRootSignature                    = rs.GetInternal().Get();

        ComPtr<ID3D12PipelineState> pso;
        ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));

        DescriptorHeap tempHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Width               = inscatterMapWidth;
        texDesc.Height              = inscatterMapHeight;
        texDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        texDesc.Format              = format;
        texDesc.MipLevels           = 1;
        texDesc.DepthOrArraySize    = inscatterMapDepth;
        texDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        texDesc.SampleDesc.Count    = 1;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;

        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&deltaSR)));
        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&deltaSM)));
        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&deltaJ)));
        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&_inscatterTable)));

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format                           = format;
        uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.WSize                  = inscatterMapDepth;

        FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(deltaSR.Get(), nullptr, &uavDesc, heapHandle.handle);

        heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(deltaSM.Get(), nullptr, &uavDesc, heapHandle.handle);

        heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(_inscatterTable.Get(), nullptr, &uavDesc, heapHandle.handle);
        
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture3D.WSize = 0;

        heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(transmittance.Get(), nullptr, &uavDesc, heapHandle.handle);

        CommandList cmdList{CommandListType::Direct, device, pso};

        auto heap = tempHeap.GetResource().Get();
        cmdList->SetDescriptorHeaps(1, &heap);
        cmdList->SetComputeRootSignature(rs.GetInternal().Get());
        cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(0));
        cmdList->Dispatch(inscatterMapWidth, inscatterMapHeight, inscatterMapDepth);
        cmdList.Close();

        ExecuteCommandList(cmdList);
    }

    // Single irradiance computing (deltaE table)
    {
        PIXScopedEvent(_deviceResources->GetCommandQueue().Get(), 0, "Single irradiance (deltaE)");

        auto                  content = ReadFileContent("SingleIrradianceCompute.cso");
        D3D12_SHADER_BYTECODE bc      = {};
        bc.BytecodeLength             = content.size();
        bc.pShaderBytecode            = content.data();

        RootSignature rs;
        rs.Init(1, 0);
        rs[0].InitAsDescriptorsTable(1);
        rs[0].InitTableRange(0, 0, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
        rs.Finalize(device);

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.CS                                = bc;
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.pRootSignature                    = rs.GetInternal().Get();

        ComPtr<ID3D12PipelineState> pso;
        ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));

        DescriptorHeap tempHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Width               = irradianceMapWidth;
        texDesc.Height              = irradianceMapHeight;
        texDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Format              = format;
        texDesc.MipLevels           = 1;
        texDesc.DepthOrArraySize    = 1;
        texDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        texDesc.SampleDesc.Count    = 1;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;

        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&deltaE)));

        // this table is not needed here, creating it here because description structure is the same as for deltaE
        ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&_irradianceTable)));

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format                           = format;
        uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;

        FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(deltaE.Get(), nullptr, &uavDesc, heapHandle.handle);

        heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(transmittance.Get(), nullptr, &uavDesc, heapHandle.handle);

        CommandList cmdList{CommandListType::Direct, device, pso};

        auto heap = tempHeap.GetResource().Get();
        cmdList->SetDescriptorHeaps(1, &heap);
        cmdList->SetComputeRootSignature(rs.GetInternal().Get());
        cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(0));
        cmdList->Dispatch(irradianceMapWidth, irradianceMapHeight, 1);
        cmdList.Close();

        ExecuteCommandList(cmdList);
    }

    // Copy deltaE to irradiance
    // TODO (DB): is this first copy needed? Bruneton uses weird k = 0 for that one, it should not copy anything
    // It needs further investigation
    {
        PIXScopedEvent(_deviceResources->GetCommandQueue().Get(), 0, "Copy deltaE");

        auto                  content = ReadFileContent("CopyInscatterCompute.cso");
        D3D12_SHADER_BYTECODE bc      = {};
        bc.BytecodeLength             = content.size();
        bc.pShaderBytecode            = content.data();

        copyTextureRS.Init(2, 0);
        copyTextureRS[0].InitAsDescriptorsTable(1);
        copyTextureRS[0].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
        copyTextureRS[1].InitAsDescriptorsTable(1);
        copyTextureRS[1].InitTableRange(0, 1, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
        copyTextureRS.Finalize(device);

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.CS                                = bc;
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.pRootSignature                    = copyTextureRS.GetInternal().Get();

        ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&copyDeltaEPso)));

        DescriptorHeap tempHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format                           = format;
        uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;

        FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(_irradianceTable.Get(), nullptr, &uavDesc, heapHandle.handle);

        heapHandle = tempHeap.GetFreeCPUAddress();
        device->CreateUnorderedAccessView(deltaE.Get(), nullptr, &uavDesc, heapHandle.handle);

        CommandList cmdList{CommandListType::Direct, device, copyDeltaEPso};

        auto heap = tempHeap.GetResource().Get();
        cmdList->SetDescriptorHeaps(1, &heap);
        cmdList->SetComputeRootSignature(copyTextureRS.GetInternal().Get());
        cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(0));
        cmdList->SetComputeRootDescriptorTable(1, tempHeap.GetGPUAddress(1));
        cmdList->Dispatch(irradianceMapWidth, irradianceMapHeight, 1);
        cmdList.Close();

        ExecuteCommandList(cmdList);
    }

    // now it is time to calculate multiple orders of the scrattering (2 -> 4)
    {
        PIXScopedEvent(_deviceResources->GetCommandQueue().Get(), 0, "Higher scattering orders");
        ComPtr<ID3D12PipelineState> deltaJPso;
        RootSignature               deltaJRS;
        ComPtr<ID3D12PipelineState> deltaEPso;
        RootSignature               deltaERS;
        ComPtr<ID3D12PipelineState> deltaSRPso;
        RootSignature               deltaSRRS;
        ComPtr<ID3D12PipelineState> copyDeltaSRPso;

        {
            // create loop states
            auto                  content = ReadFileContent("SphereInscatterCompute.cso");
            D3D12_SHADER_BYTECODE bc      = {};
            bc.BytecodeLength             = content.size();
            bc.pShaderBytecode            = content.data();

            deltaJRS.Init(3, 0);
            deltaJRS[0].InitAsDescriptorsTable(1);
            deltaJRS[0].InitTableRange(0, 0, 3, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
            deltaJRS[1].InitAsDescriptorsTable(1);
            deltaJRS[1].InitTableRange(0, 3, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
            deltaJRS[2].InitAsConstants(1, 0);
            deltaJRS.Finalize(device);

            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
            desc.CS                                = bc;
            desc.pRootSignature                    = deltaJRS.GetInternal().Get();

            ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&deltaJPso)));

            content            = ReadFileContent("SphereIrradianceCompute.cso");
            bc                 = {};
            bc.BytecodeLength  = content.size();
            bc.pShaderBytecode = content.data();

            deltaERS.Init(3, 0);
            deltaERS[0].InitAsDescriptorsTable(1);
            deltaERS[0].InitTableRange(0, 0, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
            deltaERS[1].InitAsDescriptorsTable(1);
            deltaERS[1].InitTableRange(0, 2, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
            deltaERS[2].InitAsConstants(1, 0);
            deltaERS.Finalize(device);

            desc.CS             = bc;
            desc.pRootSignature = deltaERS.GetInternal().Get();

            ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&deltaEPso)));

            content = ReadFileContent("SphereNInscatterCompute.cso");
            bc = {};
            bc.BytecodeLength = content.size();
            bc.pShaderBytecode = content.data();

            deltaSRRS.Init(2, 0);
            deltaSRRS[0].InitAsDescriptorsTable(1);
            deltaSRRS[0].InitTableRange(0, 0, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
            deltaSRRS[1].InitAsDescriptorsTable(1);
            deltaSRRS[1].InitTableRange(0, 2, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
            deltaSRRS.Finalize(device);

            desc.CS = bc;
            desc.pRootSignature = deltaSRRS.GetInternal().Get();

            ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&deltaSRPso)));

            content = ReadFileContent("CopyInscatterCompute.cso");
            bc = {};
            bc.BytecodeLength = content.size();
            bc.pShaderBytecode = content.data();

            desc.CS = bc;
            desc.pRootSignature = copyTextureRS.GetInternal().Get();

            ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&copyDeltaSRPso)));
        }
        {
            DescriptorHeap tempHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16);

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format                           = format;
            uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.WSize                  = inscatterMapDepth;

            FreeAddress<D3D12_CPU_DESCRIPTOR_HANDLE> heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(deltaSM.Get(), nullptr, &uavDesc, heapHandle.handle);

            heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(deltaSR.Get(), nullptr, &uavDesc, heapHandle.handle);

            heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(deltaJ.Get(), nullptr, &uavDesc, heapHandle.handle);

            heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(_inscatterTable.Get(), nullptr, &uavDesc, heapHandle.handle);

            uavDesc.ViewDimension   = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture3D.WSize = 0;

            heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(transmittance.Get(), nullptr, &uavDesc, heapHandle.handle);

            heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(deltaE.Get(), nullptr, &uavDesc, heapHandle.handle);

            heapHandle = tempHeap.GetFreeCPUAddress();
            device->CreateUnorderedAccessView(_irradianceTable.Get(), nullptr, &uavDesc, heapHandle.handle);

            for (int i = 2; i <= maxScatteringOrder; ++i)
            {
                PIXScopedEvent(_deviceResources->GetCommandQueue().Get(), 0, "Order %d", i);

                int first = i == 2 ? 1 : 0;  // used to apply phase functions

                CommandList cmdList{ CommandListType::Direct, device };

                auto heap = tempHeap.GetResource().Get();
                cmdList->SetPipelineState(deltaJPso.Get());
                cmdList->SetDescriptorHeaps(1, &heap);
                cmdList->SetComputeRootSignature(deltaJRS.GetInternal().Get());
                cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(0));
                cmdList->SetComputeRootDescriptorTable(1, tempHeap.GetGPUAddress(4)); // transmittance
                cmdList->SetComputeRoot32BitConstant(2, first, 0);
                cmdList->Dispatch(inscatterMapWidth, inscatterMapHeight, inscatterMapDepth);

                cmdList->SetPipelineState(deltaEPso.Get());
                cmdList->SetComputeRootSignature(deltaERS.GetInternal().Get());
                cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(0)); // delta SM/SR
                cmdList->SetComputeRootDescriptorTable(1, tempHeap.GetGPUAddress(5)); // deltaE
                cmdList->SetComputeRoot32BitConstant(2, first, 0);
                cmdList->Dispatch(irradianceMapWidth, irradianceMapHeight, 1);

                cmdList->SetPipelineState(deltaSRPso.Get());
                cmdList->SetComputeRootSignature(deltaSRRS.GetInternal().Get());
                cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(1)); // delta SR/J
                cmdList->SetComputeRootDescriptorTable(1, tempHeap.GetGPUAddress(4)); // transmittance
                cmdList->Dispatch(inscatterMapWidth, inscatterMapHeight, inscatterMapDepth);

                cmdList->SetPipelineState(copyDeltaEPso.Get());
                cmdList->SetComputeRootSignature(copyTextureRS.GetInternal().Get());
                cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(6)); // irradiance
                cmdList->SetComputeRootDescriptorTable(1, tempHeap.GetGPUAddress(5)); // deltaE
                cmdList->Dispatch(irradianceMapWidth, irradianceMapHeight, 1);

                cmdList->SetPipelineState(copyDeltaSRPso.Get());
                cmdList->SetComputeRootSignature(copyTextureRS.GetInternal().Get());
                cmdList->SetComputeRootDescriptorTable(0, tempHeap.GetGPUAddress(3)); // inscatter
                cmdList->SetComputeRootDescriptorTable(1, tempHeap.GetGPUAddress(1)); // deltaSR
                cmdList->Dispatch(inscatterMapWidth, inscatterMapHeight, inscatterMapDepth);

                cmdList.Close();

                ExecuteCommandList(cmdList);
            }
        }
    }
}
