#include "stdafx.h"

#include "SceneManager.h"

#include <shaders/Common.h>
#include <utils/RenderTargetManager.h>
#include <utils/Shaders.h>

#include <filesystem>
#include <iostream>
#include <random>

constexpr auto pi = 3.14159265f;

// Hit groups.
const wchar_t* hitGroupNames[] = {L"MyHitGroup_Triangle"};
const wchar_t* closestHitShaderNames[] = {L"ClosestHitShader_Triangle"};

static constexpr uint32_t slocalRootSignatureBindingsCount = 3;
size_t hitTableStride = 0;

constexpr std::size_t AlignTo(std::size_t size, std::size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

SceneManager::SceneManager(std::shared_ptr<DeviceResources>           deviceResources,
                           UINT                                       screenWidth,
                           UINT                                       screenHeight,
                           CommandLineOptions                         cmdLineOpts,
                           RenderTargetManager*                       rtManager,
                           std::vector<std::shared_ptr<RenderTarget>> swapChainRTs)
    : _deviceResources(deviceResources)
    , _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
    , _rtManager(rtManager)
    , _swapChainRTs(swapChainRTs)
    , _mainCamera(Graphics::ProjectionType::Perspective, 0.1f, 1000.f, 5.0f * pi / 18.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight))
    , _meshManager(_deviceResources->GetDevice())
{
    assert(rtManager);

    _mainCamera.SetCenter({ 0.0f, 0.0f, 0.0f });
    _mainCamera.SetRadius(5.0f);

    SetThreadDescription(GetCurrentThread(), L"Main thread");

    CreateFrameResources();
    CreateRenderTargets();
    CreateRootSignatures();
    CreateRaytracingPSO();
    CreateCommandLists();

    auto executor = [this](CommandList& cmdList) { ExecuteCommandList(cmdList); };

    _sceneObjects.push_back(std::make_shared<SceneObject>(_meshManager.CreateEmptyCube(executor), deviceResources->GetDevice()));
    _sceneObjects.back()->Rotation(0.5f);

    _sceneObjects.push_back(std::make_shared<SceneObject>(_meshManager.CreateCube(executor), deviceResources->GetDevice()));
    _sceneObjects.back()->Position({2.0, 0.0, 0.0});

     CreateShaderTables();

    BuildTLAS();
}

SceneManager::~SceneManager()
{
    _deviceResources->WaitForCurrentFrame();
}

void SceneManager::DrawScene()
{
    std::vector<ID3D12CommandList*> cmdListArray;
    cmdListArray.reserve(16); // just to avoid any allocations
    UpdateObjects();
    PopulateCommandList();
    cmdListArray.push_back(_cmdList->GetInternal().Get());

    _deviceResources->GetCommandQueue()->ExecuteCommandLists(1, cmdListArray.data());
}

void SceneManager::Present()
{
    // Swap buffers
    _deviceResources->GetSwapChain()->Present(0, 0);
    _deviceResources->WaitForCurrentFrame();
}

void SceneManager::ExecuteCommandList(const CommandList& commandList)
{
    std::array<ID3D12CommandList*, 1> cmdListsArray = {commandList.GetInternal().Get()};
    _deviceResources->GetCommandQueue()->ExecuteCommandLists((UINT)cmdListsArray.size(), cmdListsArray.data());
    _deviceResources->WaitForCurrentFrame();
}

Graphics::SphericalCamera& SceneManager::GetCamera()
{
    return _mainCamera;
}

void SceneManager::SetLightColor(float r, float g, float b)
{
    _lightColors[0] = r;
    _lightColors[1] = g;
    _lightColors[2] = b;
}

void SceneManager::SetLightPos(float x, float y, float z)
{
    _lightPos[0] = x;
    _lightPos[1] = y;
    _lightPos[2] = z;
}

void SceneManager::SetAmbientColor(float r, float g, float b)
{
    _ambientColor[0] = r;
    _ambientColor[1] = g;
    _ambientColor[2] = b;
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

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 100; // it should be enough :)
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(_deviceResources->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtsHeap)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    _deviceResources->GetDevice()->CreateUnorderedAccessView(_raytracingOutput.Get(), nullptr, &uavDesc, _rtsHeap->GetCPUDescriptorHandleForHeapStart());

    // move resources into correct states to avoid DXDebug errors on first barrier calls
    //std::array<D3D12_RESOURCE_BARRIER, 5> barriers = {
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtDepth->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)
    //};

    //CommandList temporaryCmdList {CommandListType::Direct, _deviceResources->GetDevice()};
    //temporaryCmdList.GetInternal()->ResourceBarrier((UINT)barriers.size(), barriers.data());
    //temporaryCmdList.Close();

    //ExecuteCommandLists(temporaryCmdList);
}

void SceneManager::CreateShaderTables()
{
    CreateConstantBuffer(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &_missTable, D3D12_RESOURCE_STATE_GENERIC_READ);
    CreateConstantBuffer(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &_raygenTable, D3D12_RESOURCE_STATE_GENERIC_READ);

    std::size_t stride = (D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + slocalRootSignatureBindingsCount * sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    stride = AlignTo(stride, 64);

    std::size_t size = _sceneObjects.size() * stride;
    CreateConstantBuffer(size, &_hitTable, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void SceneManager::PopulateCommandList()
{
    ComPtr<ID3D12StateObjectProperties> props;
    ThrowIfFailed(_raytracingState->QueryInterface(props.GetAddressOf()));

    void* raygenTableData = nullptr;
    ThrowIfFailed(_raygenTable->Map(0, nullptr, &raygenTableData));
    memcpy(raygenTableData, props->GetShaderIdentifier(L"RayGenShader"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    void* missTableData = nullptr;
    ThrowIfFailed(_missTable->Map(0, nullptr, &missTableData));
    memcpy(missTableData, props->GetShaderIdentifier(L"MissShader"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    void* hitTableData = nullptr;
    ThrowIfFailed(_hitTable->Map(0, nullptr, &hitTableData));
    for (std::size_t i = 0; i < _sceneObjects.size(); i++)
    {
        std::size_t stride = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + slocalRootSignatureBindingsCount * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        stride = AlignTo(stride, 64);
        uint8_t* destination = reinterpret_cast<uint8_t*>(hitTableData) + i * stride;

        void* hitGroupShaderIdentifier = props->GetShaderIdentifier(hitGroupNames[0]);
        memcpy(destination, hitGroupShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        
        auto gpuVirtualAddress = _sceneObjects[i]->GetConstantBuffer()->GetGPUVirtualAddress();
        auto dest              = destination + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        memcpy(dest, &gpuVirtualAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
        
        auto vertexGpuAddress = _sceneObjects[i]->GetVertexBuffer()->GetGPUVirtualAddress();
        dest += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        memcpy(dest, &vertexGpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
        
        auto indexGpuAddress = _sceneObjects[i]->GetIndexBuffer()->GetGPUVirtualAddress();
        dest += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        memcpy(dest, &indexGpuAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
    }

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.Height = _screenHeight;
    desc.Width = _screenWidth;
    desc.Depth = 1;

    desc.RayGenerationShaderRecord.StartAddress = _raygenTable->GetGPUVirtualAddress();
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    desc.MissShaderTable.StartAddress = _missTable->GetGPUVirtualAddress();
    desc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    desc.MissShaderTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    std::size_t stride = AlignTo(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + slocalRootSignatureBindingsCount * sizeof(D3D12_GPU_VIRTUAL_ADDRESS), 64);
    desc.HitGroupTable.StartAddress = _hitTable->GetGPUVirtualAddress();
    desc.HitGroupTable.SizeInBytes   = stride * _sceneObjects.size();
    desc.HitGroupTable.StrideInBytes = stride;

    _cmdList->Reset();
    _cmdList->GetInternal()->SetPipelineState1(_raytracingState.Get());

    ID3D12DescriptorHeap* heap = _rtsHeap.Get();
    _cmdList->GetInternal()->SetDescriptorHeaps(1, &heap);
    _cmdList->GetInternal()->SetComputeRootSignature(_globalRootSignature.GetInternal().Get());
    _cmdList->GetInternal()->SetComputeRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());
    _cmdList->GetInternal()->SetComputeRootConstantBufferView(1, _viewParams->GetGPUVirtualAddress());
    _cmdList->GetInternal()->SetComputeRootConstantBufferView(2, _lightParams->GetGPUVirtualAddress());
    _cmdList->GetInternal()->DispatchRays(&desc);

    size_t frameIndex = _deviceResources->GetSwapChain()->GetCurrentBackBufferIndex();
    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        _cmdList->GetInternal()->ResourceBarrier(1, &transition);
    }

    auto rtBuffer = _swapChainRTs[frameIndex].get();
    _rtManager->ClearRenderTarget(*rtBuffer, *_cmdList);

    {
        std::vector< CD3DX12_RESOURCE_BARRIER> transitions(2);
        transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        _cmdList->GetInternal()->ResourceBarrier(transitions.size(), transitions.data());
    }

    _cmdList->GetInternal()->CopyResource(rtBuffer->_texture.Get(), _raytracingOutput.Get());

    // Indicate that the back buffer will be used as a render target.
    {
        std::vector< CD3DX12_RESOURCE_BARRIER> transitions(2);
        transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        _cmdList->GetInternal()->ResourceBarrier(transitions.size(), transitions.data());
    }

    _cmdList->Close();
}

void SceneManager::CreateCommandLists()
{
    _cmdList = std::make_unique<CommandList>(CommandListType::Direct, _deviceResources->GetDevice());
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

std::vector<char> readBytecode(const std::string& fileName)
{
    std::ifstream file;
    if (!std::filesystem::exists(fileName))
    {
        throw std::runtime_error("File " + fileName + " doesn't exist");
    }

    file.open(fileName, std::ifstream::binary);

    file.seekg(0, file.end);
    long bytecodeLength = file.tellg();
    file.seekg(0, file.beg);

    std::vector<char> bytecode(bytecodeLength);
    file.read(bytecode.data(), bytecodeLength);
    return bytecode;
}

void SceneManager::CreateRaytracingPSO()
{
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;

    D3D12_DXIL_LIBRARY_DESC library_desc = {};

    D3D12_SHADER_BYTECODE bytecode;
    std::vector<char> bytes = readBytecode("Sample.cso");

    library_desc.DXILLibrary.pShaderBytecode = bytes.data();
    library_desc.DXILLibrary.BytecodeLength = bytes.size();

    std::vector<D3D12_EXPORT_DESC> exports(3);
    exports[0].Name = L"RayGenShader";
    exports[1].Name = L"MissShader";
    exports[2].Name = closestHitShaderNames[0];

    library_desc.NumExports = exports.size();
    library_desc.pExports = exports.data();
    {
        // library
        D3D12_STATE_SUBOBJECT library;
        library.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;

        library.pDesc = &library_desc;
        subobjects.emplace_back(library);
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc   = {};
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE::D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.HitGroupExport = hitGroupNames[0];
    hitGroupDesc.ClosestHitShaderImport = closestHitShaderNames[0];
    {
        D3D12_STATE_SUBOBJECT hitGroupSubobject;
        hitGroupSubobject.Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroupSubobject.pDesc = &hitGroupDesc;
        subobjects.emplace_back(hitGroupSubobject);
    }

    D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig;
    descPipelineConfig.MaxTraceRecursionDepth = 1;
    {
        D3D12_STATE_SUBOBJECT pipelineSubobject;
        pipelineSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        pipelineSubobject.pDesc = &descPipelineConfig;
        subobjects.emplace_back(pipelineSubobject);
    }

    D3D12_RAYTRACING_SHADER_CONFIG  descShaderConfig;
    descShaderConfig.MaxPayloadSizeInBytes = sizeof(float)*4; 
    descShaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES	;
    {
        D3D12_STATE_SUBOBJECT shaderSubobject;
        shaderSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        shaderSubobject.pDesc = &descShaderConfig;
        subobjects.emplace_back(shaderSubobject);
    }

    D3D12_GLOBAL_ROOT_SIGNATURE grsDesc;
    grsDesc.pGlobalRootSignature = _globalRootSignature.GetInternal().Get();
    {
        D3D12_STATE_SUBOBJECT globalRootSignature;
        globalRootSignature.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        globalRootSignature.pDesc = &grsDesc;
        subobjects.emplace_back(globalRootSignature);
    }

    D3D12_LOCAL_ROOT_SIGNATURE lrsDesc;
    lrsDesc.pLocalRootSignature = _localRootSignature.GetInternal().Get();
    {
        D3D12_STATE_SUBOBJECT localRootSignature;
        localRootSignature.Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        localRootSignature.pDesc = &lrsDesc;
        subobjects.emplace_back(localRootSignature);
    }

    D3D12_STATE_OBJECT_DESC pDesc;
    pDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    pDesc.NumSubobjects = subobjects.size();
    pDesc.pSubobjects = subobjects.data();

    ThrowIfFailed(_deviceResources->GetDevice()->CreateStateObject(&pDesc, IID_PPV_ARGS(&_raytracingState)));
}

void SceneManager::CreateRootSignatures()
{
    _globalRootSignature.Init(3, 0);
    _globalRootSignature[0].InitAsDescriptorsTable(2); // Output UAV
    _globalRootSignature[0].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
    _globalRootSignature[0].InitTableRange(1, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    _globalRootSignature[1].InitAsCBV(0);
    _globalRootSignature[2].InitAsCBV(1);
    _globalRootSignature.Finalize(_deviceResources->GetDevice());

    _localRootSignature.Init(3, 0);
    _localRootSignature[0].InitAsCBV(2);
    _localRootSignature[1].InitAsSRV(1);
    _localRootSignature[2].InitAsSRV(2);
    _localRootSignature.Finalize(_deviceResources->GetDevice(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void SceneManager::CreateFrameResources()
{
    CreateConstantBuffer(sizeof(ViewParams), &_viewParams, D3D12_RESOURCE_STATE_GENERIC_READ);
    CreateConstantBuffer(sizeof(LightParams), &_lightParams, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void SceneManager::UpdateObjects()
{
    void* sceneData = nullptr;

    ThrowIfFailed(_viewParams->Map(0, nullptr, &sceneData));

    DirectX::XMFLOAT4 camPos = _mainCamera.GetEyePosition();
    DirectX::XMVECTOR m      = {camPos.x, camPos.y, camPos.z, camPos.w};

    DirectX::XMMATRIX viewProjMat = _mainCamera.GetViewProjMatrix();

    ((ViewParams*)sceneData)->viewPos         = m;
    ((ViewParams*)sceneData)->inverseViewProj = DirectX::XMMatrixInverse(nullptr, viewProjMat);
    ((ViewParams*)sceneData)->ambientColor    = DirectX::XMVECTOR({ _ambientColor[0], _ambientColor[1], _ambientColor[2], 1.0 });

    ThrowIfFailed(_lightParams->Map(0, nullptr, &sceneData));

    ((LightParams*)sceneData)->direction = DirectX::XMVECTOR({ _lightPos[0], _lightPos[1], _lightPos[2], 1.0});
    ((LightParams*)sceneData)->color     = DirectX::XMVECTOR({_lightColors[0], _lightColors[1], _lightColors[2], 1.0});

    _sceneObjects.back()->Rotation(_sceneObjects.back()->Rotation() + 0.01f);

    bool anyDirty = std::any_of(_sceneObjects.cbegin(), _sceneObjects.cend(),
                                [](const SceneObjectPtr& object) { return object->IsDirty(); });

    if (anyDirty)
    {
        BuildTLAS();
        std::for_each(_sceneObjects.begin(), _sceneObjects.end(), [](SceneObjectPtr& object) { object->ResetDirty(); });
    }
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
        instanceDesc.AccelerationStructure               = _sceneObjects[idx]->GetBLAS()->GetGPUVirtualAddress();
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

    cmdList.GetInternal()->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    cmdList.Close();

    ExecuteCommandList(cmdList);

    D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = _rtsHeap->GetCPUDescriptorHandleForHeapStart();
    heapHandle.ptr += _deviceResources->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.RaytracingAccelerationStructure.Location = _tlas->GetGPUVirtualAddress();
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    _deviceResources->GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, heapHandle);
}
