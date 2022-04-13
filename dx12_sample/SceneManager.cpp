#include "stdafx.h"

#include "Common.h"
#include "SceneManager.h"

#include <utils/RenderTargetManager.h>
#include <utils/Shaders.h>

#include <random>
#include <iostream>

#include <filesystem>

constexpr auto pi = 3.14159265f;

SceneManager::SceneManager(ComPtr<ID3D12Device5> pDevice,
    UINT screenWidth,
    UINT screenHeight,
    CommandLineOptions cmdLineOpts,
    ComPtr<ID3D12CommandQueue> pCmdQueue,
    ComPtr<IDXGISwapChain3> pSwapChain,
    RenderTargetManager* rtManager)
    : _device(pDevice)
    , _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
    , _cmdQueue(pCmdQueue)
    , _swapChain(pSwapChain)
    , _frameIndex(pSwapChain->GetCurrentBackBufferIndex())
    , _swapChainRTs(_swapChainRTs)
    , _rtManager(rtManager)
    , _mainCamera(Graphics::ProjectionType::Perspective, 0.1f, 1000.f, 5.0f * pi / 18.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight))
{
    assert(pDevice);
    assert(pCmdQueue);
    assert(pSwapChain);
    assert(rtManager);

    _mainCamera.SetCenter({ 0.0f, 0.0f, 0.0f });
    _mainCamera.SetRadius(5.0f);

    SetThreadDescription(GetCurrentThread(), L"Main thread");

    // Have to create Fence and Event.
    ThrowIfFailed(pDevice->CreateFence(0,
                                       D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS(&_frameFence)));
    _frameEndEvent = CreateEvent(NULL, FALSE, FALSE, nullptr);

    CreateFrameResources();
    CreateRenderTargets();
    CreateRootSignatures();
    CreateRaytracingPSO();
    CreateShaderTables();
    CreateCommandLists();
}

SceneManager::~SceneManager()
{
    WaitCurrentFrame();
}

void SceneManager::DrawAll()
{
    std::vector<ID3D12CommandList*> cmdListArray;
    cmdListArray.reserve(16); // just to avoid any allocations
    UpdateObjects();
    PopulateCommandList();
    cmdListArray.push_back(_cmdList->GetInternal().Get());

    _cmdQueue->ExecuteCommandLists(1, cmdListArray.data());

    // Swap buffers
    _swapChain->Present(0, 0);
    WaitCurrentFrame();
}

void SceneManager::ExecuteCommandLists(const CommandList & commandList)
{
    {
        PIXScopedEvent(_cmdQueue.Get(), 0, "Submitting");
        std::array<ID3D12CommandList*, 1> cmdListsArray = { commandList.GetInternal().Get() };
        _cmdQueue->ExecuteCommandLists((UINT)cmdListsArray.size(), cmdListsArray.data());
    }

    _fenceValue++;
    WaitCurrentFrame();
}

Graphics::SphericalCamera& SceneManager::GetCamera()
{
    return _mainCamera;
}

void SceneManager::WaitCurrentFrame()
{
    PIXScopedEvent(_cmdQueue.Get(), 0, "Waiting for a fence");
    uint64_t newFenceValue = _fenceValue;

    ThrowIfFailed(_cmdQueue->Signal(_frameFence.Get(), newFenceValue));
    _fenceValue++;

    if (_frameFence->GetCompletedValue() != newFenceValue)
    {
        ThrowIfFailed(_frameFence->SetEventOnCompletion(newFenceValue, _frameEndEvent));
        WaitForSingleObject(_frameEndEvent, INFINITE);
    }

    _frameIndex = _swapChain->GetCurrentBackBufferIndex();
}

void SceneManager::CreateRenderTargets()
{
    _swapChainRTs = _rtManager->CreateRenderTargetsForSwapChain(_swapChain);

    D3D12_RESOURCE_DESC dxrOutputDesc = {};
    dxrOutputDesc.Width = _screenWidth;
    dxrOutputDesc.Height = _screenHeight;
    dxrOutputDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dxrOutputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxrOutputDesc.MipLevels = 1;
    dxrOutputDesc.DepthOrArraySize = 1;
    dxrOutputDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    dxrOutputDesc.SampleDesc.Count = 1;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    ThrowIfFailed(_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &dxrOutputDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&_raytracingOutput)));

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 100; // it should be enough :)
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtsHeap)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    _device->CreateUnorderedAccessView(_raytracingOutput.Get(), nullptr, &uavDesc, _rtsHeap->GetCPUDescriptorHandleForHeapStart());

    // move resources into correct states to avoid DXDebug errors on first barrier calls
    //std::array<D3D12_RESOURCE_BARRIER, 5> barriers = {
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    //    CD3DX12_RESOURCE_BARRIER::Transition(_mrtDepth->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)
    //};

    //CommandList temporaryCmdList {CommandListType::Direct, _device};
    //temporaryCmdList.GetInternal()->ResourceBarrier((UINT)barriers.size(), barriers.data());
    //temporaryCmdList.Close();

    //ExecuteCommandLists(temporaryCmdList);
}

void SceneManager::CreateShaderTables()
{
    CreateConstantBuffer(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &_missTable);
    CreateConstantBuffer(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &_raygenTable);
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

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.Height = _screenHeight;
    desc.Width = _screenWidth;
    desc.Depth = 1;
    desc.RayGenerationShaderRecord.StartAddress = _raygenTable->GetGPUVirtualAddress();
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    desc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    desc.MissShaderTable.StartAddress = _missTable->GetGPUVirtualAddress();
    desc.MissShaderTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    _cmdList->Reset();
    _cmdList->GetInternal()->SetPipelineState1(_raytracingState.Get());

    ID3D12DescriptorHeap* heap = _rtsHeap.Get();
    _cmdList->GetInternal()->SetDescriptorHeaps(1, &heap);
    _cmdList->GetInternal()->SetComputeRootSignature(_globalRootSignature.GetInternal().Get());
    _cmdList->GetInternal()->SetComputeRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());
    _cmdList->GetInternal()->SetComputeRootConstantBufferView(1, _viewParams->GetGPUVirtualAddress());
    _cmdList->GetInternal()->DispatchRays(&desc);

    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        _cmdList->GetInternal()->ResourceBarrier(1, &transition);
    }

    auto rtBuffer = _swapChainRTs[_frameIndex].get();
    _rtManager->ClearRenderTarget(*rtBuffer, *_cmdList);

    {
        std::vector< CD3DX12_RESOURCE_BARRIER> transitions(2);
        transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        _cmdList->GetInternal()->ResourceBarrier(transitions.size(), transitions.data());
    }

    _cmdList->GetInternal()->CopyResource(rtBuffer->_texture.Get(), _raytracingOutput.Get());

    // Indicate that the back buffer will be used as a render target.
    {
        std::vector< CD3DX12_RESOURCE_BARRIER> transitions(2);
        transitions[0] = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
        transitions[1] = CD3DX12_RESOURCE_BARRIER::Transition(_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        _cmdList->GetInternal()->ResourceBarrier(transitions.size(), transitions.data());
    }

    _cmdList->Close();
}

void SceneManager::CreateCommandLists()
{
    _cmdList = std::make_unique<CommandList>(CommandListType::Direct, _device);
    _cmdList->Close();
}

void SceneManager::CreateConstantBuffer(size_t bufferSize, ComPtr<ID3D12Resource>* pOutBuffer)
{
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = bufferSize;
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProp = { D3D12_HEAP_TYPE_UPLOAD };
    ThrowIfFailed(_device->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&(*pOutBuffer))));
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

    std::vector<D3D12_EXPORT_DESC> exports(2);
    exports[0].Name = L"RayGenShader";
    exports[1].Name = L"MissShader";

    library_desc.NumExports = exports.size();
    library_desc.pExports = exports.data();
    {
        // library
        D3D12_STATE_SUBOBJECT library;
        library.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;

        library.pDesc = &library_desc;
        subobjects.emplace_back(library);
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

    D3D12_STATE_OBJECT_DESC pDesc;
    pDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    pDesc.NumSubobjects = subobjects.size();
    pDesc.pSubobjects = subobjects.data();

    ThrowIfFailed(_device->CreateStateObject(&pDesc, IID_PPV_ARGS(&_raytracingState)));
}

void SceneManager::CreateRootSignatures()
{
    _globalRootSignature.Init(2, 0);
    _globalRootSignature[0].InitAsDescriptorsTable(2); // Output UAV
    _globalRootSignature[0].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
    _globalRootSignature[0].InitTableRange(1, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    _globalRootSignature[1].InitAsCBV(0);
    _globalRootSignature.Finalize(_device);
}

void SceneManager::CreateFrameResources()
{
    CreateConstantBuffer(sizeof(ViewParams), &_viewParams);
}

void SceneManager::UpdateObjects()
{
    void* sceneData = nullptr;
    ThrowIfFailed(_viewParams->Map(0, nullptr, &sceneData));

    DirectX::XMFLOAT4 camPos = _mainCamera.GetEyePosition();
    DirectX::XMVECTOR m = { camPos.x, camPos.y, camPos.z, camPos.w };

    DirectX::XMMATRIX projMat = _mainCamera.GetProjMatrix();

    ((ViewParams*)sceneData)->viewPos = m;
    ((ViewParams*)sceneData)->inverseViewProj = DirectX::XMMatrixInverse(nullptr, projMat);
}
