#include "stdafx.h"

#include "DX12Sample.h"
#include "worldgen/TerrainManager.h"

#include <utils/CommandList.h>
#include <utils/FeaturesCollector.h>
#include <utils/Math.h>

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>

constexpr std::size_t mapSize = Math::AlignTo(1024, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

using namespace std::chrono;

DX12Sample::DX12Sample(int windowWidth, int windowHeight, std::set<optTypes>& opts)
    : DXSample(windowWidth, windowHeight, L"HELLO YOPTA")
    , _worldGen(mapSize)
{
}

DX12Sample::~DX12Sample()
{
    // explicitly here
    _sceneManager.reset();
}

void DX12Sample::OnInit()
{
#ifdef _DEBUG
    // Enable the D3D12 debug layer.
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    _deviceResources = CreateDeviceResources();
    DumpFeatures();

    GameInput::Initialize(m_hwnd);
    GameInput::AcquireMouse();

    _uiCmdList = std::make_unique<CommandList>(CommandListType::Direct, _deviceResources->GetDevice());
    _uiCmdList->Close();

    _RTManager    = std::make_unique<RenderTargetManager>(_deviceResources->GetDevice());
    auto swapChainRts = _RTManager->CreateRenderTargetsForSwapChain(_deviceResources->GetSwapChain());
    _deviceResources->SetSwapChainRts(swapChainRts);

    _sceneManager = std::make_unique<SceneManager>(_deviceResources, m_width, m_height, _cmdLineOpts, _RTManager.get());

    D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
    imguiHeapDesc.NumDescriptors = 32;
    imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(_deviceResources->GetDevice()->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&_uiDescriptors)));

    CreateUITexture();
    _worldGen.GenerateHeightMap(_worldGenParams.octaves, _worldGenParams.persistance, _worldGenParams.frequency,
                                _worldGenParams.lacunarity);
    UpdateWorldTexture();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer back ends
    ImGui_ImplWin32_Init(DXSample::m_hwnd);
    ImGui_ImplDX12_Init(_deviceResources->GetDevice().Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, _uiDescriptors.Get(),
                        _uiDescriptors->GetCPUDescriptorHandleForHeapStart(),
                        _uiDescriptors->GetGPUDescriptorHandleForHeapStart());

    _camera = _sceneManager->CreateWASDCamera();
    _camera->SetPosition({mapSize / 2.0f, mapSize / 2.0f, (float)_worldGen.GetHeight(mapSize / 2, mapSize / 2) + 25.0f});

    CreateObjects();
}

void DX12Sample::OnUpdate()
{
    static auto prevTime = high_resolution_clock::now();
    static int elapsedFrames = 0;
    static double elapsedTime = 0.0;

    auto diff = duration_cast<microseconds>(high_resolution_clock::now() - prevTime);
    double dt = (double)diff.count() / 1000000.0;

    GameInput::Update(dt, m_hwnd);

    prevTime = high_resolution_clock::now();
    elapsedFrames++;
    elapsedTime += dt;

    if (elapsedTime > 1.0)
    {
        for (auto i = 0; i < _frameTimes.size() - 1; ++i)
        {
            _frameTimes[i] = _frameTimes[i + 1];
            _frameTimes.back() = (float)elapsedFrames;
        }

        elapsedFrames = 0;
        elapsedTime -= 1.0;
    }

    float mult = GameInput::IsPressed(GameInput::kKey_lshift) ? 4.0f : 1.0f;
    if (GameInput::IsPressed(GameInput::kKey_w))
        _camera->MoveForward(0.1f * mult);
    if (GameInput::IsPressed(GameInput::kKey_s))
        _camera->MoveForward(-0.1f * mult);
    if (GameInput::IsPressed(GameInput::kKey_a))
        _camera->MoveSideward(0.1f * mult);
    if (GameInput::IsPressed(GameInput::kKey_d))
        _camera->MoveSideward(-0.1f * mult);
    if (GameInput::IsPressed(GameInput::kKey_q))
        _camera->MoveUpward(0.1f * mult);
    if (GameInput::IsPressed(GameInput::kKey_e))
        _camera->MoveUpward(-0.1f * mult);

    if (GameInput::IsPressed(GameInput::kMouse1))
    {
        float dx = GameInput::GetAnalogInput(GameInput::kAnalogMouseX);
        float dy = GameInput::GetAnalogInput(GameInput::kAnalogMouseY);

        _camera->RotateHorizontally(dx);
        _camera->RotateVertically(dy);
    }
}

void DX12Sample::OnRender()
{
    _sceneManager->DrawScene();

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Draw UI
    {
        int windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        const float          padding  = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        static ImVec4 ambientColor{ 0.1f, 0.1f, 0.1f, 1.0f };
        static ImVec4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
        static ImVec4 lightDir{ 0.0f, -0.154f, -0.148f, 1.0f };

        ImVec2 window_pos;
        window_pos.x = viewport->WorkPos.x + padding;
        window_pos.y = viewport->WorkPos.y + padding;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);

        ImGui::Begin("Overlay", nullptr, windowFlags);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::PlotHistogram("FPS", _frameTimes.data(), _frameTimes.size());
        ImGui::Text("Camera position:");
        ImGui::SameLine();
        ImGui::Text("X: %.3f", _camera->GetPosition().x);
        ImGui::SameLine();
        ImGui::Text("Y: %.3f", _camera->GetPosition().y);
        ImGui::SameLine();
        ImGui::Text("Z: %.3f", _camera->GetPosition().z);
        ImGui::ColorEdit3("Ambient color", (float*)&ambientColor);
        ImGui::ColorEdit3("Light color", (float*)&lightColor);
        ImGui::SliderFloat3("Light direction", (float*)&lightDir, -1.0f, 1.0f);
        ImGui::End();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Debug"))
            {
                if(ImGui::MenuItem("Show terrain controls", nullptr, _showTerrainControls))
                {
                    _showTerrainControls = !_showTerrainControls;
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }

        _sceneManager->SetAmbientColor(ambientColor.x, ambientColor.y, ambientColor.z);
        _sceneManager->SetLightColor(lightColor.x, lightColor.y, lightColor.z);
        _sceneManager->SetLightDirection(lightDir.x, lightDir.y, lightDir.z);

        if (_showTerrainControls)
        {
            ImGui::Begin("Terrain controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Image((ImTextureID)_heightMapTexId, {256, 256}, {0, 0}, {1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1});
            ImGui::SliderInt("Octaves", &_worldGenParams.octaves, 1, 10);
            ImGui::SliderFloat("Persistance", &_worldGenParams.persistance, 0.1f, 1.0f);
            ImGui::SliderFloat("Frequency", &_worldGenParams.frequency, 0.1f, 10.0f);
            ImGui::SliderFloat("Lacunarity", &_worldGenParams.lacunarity, 0.1f, 10.0f);
            if (ImGui::Button("Generate terrain"))
            {
                _worldGen.GenerateHeightMap(_worldGenParams.octaves, _worldGenParams.persistance,
                                            _worldGenParams.frequency, _worldGenParams.lacunarity);
                UpdateWorldTexture();
            }
            ImGui::End();
        }
    }

    // Rendering
    ImGui::Render();

    UINT backBufferIdx = _deviceResources->GetSwapChain()->GetCurrentBackBufferIndex();
    RenderTarget* rts[8] = {_deviceResources->GetSwapChainRts()[backBufferIdx].get()};

    _uiCmdList->Reset();
    _RTManager->BindRenderTargets(rts, nullptr, *_uiCmdList);
    _uiCmdList->GetInternal()->SetDescriptorHeaps(1, _uiDescriptors.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _uiCmdList->GetInternal().Get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = _deviceResources->GetSwapChainRts()[backBufferIdx]->_texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    _uiCmdList->GetInternal()->ResourceBarrier(1, &barrier);
    _uiCmdList->Close();

    _sceneManager->ExecuteCommandList(*_uiCmdList);
    _sceneManager->Present();
}

void DX12Sample::OnDestroy()
{
    GameInput::Shutdown();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

bool DX12Sample::OnEvent(MSG msg)
{
    switch (msg.message)
    {
    case WM_KEYDOWN:
        if (msg.wParam == VK_ESCAPE)
            return false;
        break;
    case WM_LBUTTONDOWN:
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, true);
        break;
    case WM_LBUTTONUP:
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        break;
    }

    return true;
}

void DX12Sample::HandleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOVE:
        // do not adjust size when user drags the window border
        // this will only trigger on maximization/restoring events
        if (!_isResizing)
            AdjustSizes();
        break;
    case WM_ENTERSIZEMOVE:
        _isResizing = true;
        break;
    case WM_EXITSIZEMOVE:
        _isResizing = false;
        AdjustSizes();
        break;
    case WM_ACTIVATE:
        if (wParam == WA_INACTIVE)
            GameInput::UnacquireMouse();
        if (wParam == WA_CLICKACTIVE || wParam == WA_ACTIVE)
            GameInput::AcquireMouse();
        break;
    }
}

void DX12Sample::DumpFeatures()
{
    FeaturesCollector collector {_deviceResources->GetDevice()};
    collector.CollectFeatures("deviceInfo.log");
}

void DX12Sample::UpdateWorldTexture()
{
    const std::size_t pixelSize = 4;

    // create upload buffer
    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Format              = DXGI_FORMAT_UNKNOWN;
    uploadDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
    uploadDesc.SampleDesc.Count    = 1;
    uploadDesc.DepthOrArraySize    = 1;
    uploadDesc.MipLevels           = 1;
    uploadDesc.Width               = pixelSize * mapSize * mapSize;  // 1 byte per all 4 channels, 1024x1024 pixels
    uploadDesc.Height              = 1;
    uploadDesc.Alignment           = 0;
    uploadDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES  uploadHeapProps = {D3D12_HEAP_TYPE_UPLOAD};
    ComPtr<ID3D12Resource> uploadTexture;
    ThrowIfFailed(_deviceResources->GetDevice()->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE,
                                                                         &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                         nullptr, IID_PPV_ARGS(&uploadTexture)));

    uint8_t* data = nullptr;
    ThrowIfFailed(uploadTexture->Map(0, nullptr, (void**)&data));
    for (int i = 0; i < mapSize; ++i)
    {
        for (int j = 0; j < mapSize; ++j)
        {
            uint8_t     height       = _worldGen.GetHeight(i, j);
            std::size_t rowOffset    = j * pixelSize * mapSize;
            std::size_t columnOffset = i * pixelSize;
            std::size_t pixelOffset  = rowOffset + columnOffset;

            auto color            = _colorsLut.lower_bound(height)->second;
            data[pixelOffset + 0] = color.x;
            data[pixelOffset + 1] = color.y;
            data[pixelOffset + 2] = color.z;
            data[pixelOffset + 3] = 255;  // 1.0 to alpha channel
        }
    }
    uploadTexture->Unmap(0, nullptr);

    CommandList cmdList{CommandListType::Direct, _deviceResources->GetDevice()};

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = _heightMapTexture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = 0;

    cmdList->ResourceBarrier(1, &barrier);

    // copy it to the normal GPU texture
    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex            = 0;
    dstLoc.pResource                   = _heightMapTexture.Get();

    D3D12_TEXTURE_COPY_LOCATION srcLoc        = {};
    srcLoc.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint.Offset             = 0;
    srcLoc.PlacedFootprint.Footprint.Depth    = 1;
    srcLoc.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcLoc.PlacedFootprint.Footprint.Height   = mapSize;
    srcLoc.PlacedFootprint.Footprint.RowPitch = mapSize * pixelSize;
    srcLoc.PlacedFootprint.Footprint.Width    = mapSize;
    srcLoc.pResource                          = uploadTexture.Get();

    cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    cmdList->ResourceBarrier(1, &barrier);
    cmdList.Close();
    _sceneManager->ExecuteCommandList(cmdList);
}

void DX12Sample::CreateUITexture()
{
    D3D12_RESOURCE_DESC heightMapDesc = {};
    heightMapDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    heightMapDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
    heightMapDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
    heightMapDesc.SampleDesc.Count    = 1;
    heightMapDesc.DepthOrArraySize    = 1;
    heightMapDesc.MipLevels           = 1;
    heightMapDesc.Width               = mapSize;
    heightMapDesc.Height              = mapSize;
    heightMapDesc.Alignment           = 0;

    D3D12_HEAP_PROPERTIES defaultHeapProps = {D3D12_HEAP_TYPE_DEFAULT};
    ThrowIfFailed(_deviceResources->GetDevice()->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &heightMapDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
        IID_PPV_ARGS(&_heightMapTexture)));

    // create SRV and put it into ImGUI heap
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels             = 1;

    auto incSize = _deviceResources->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    auto handle = _uiDescriptors->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += incSize;
    _deviceResources->GetDevice()->CreateShaderResourceView(_heightMapTexture.Get(), &srvDesc, handle);

    auto gpu_handle = _uiDescriptors->GetGPUDescriptorHandleForHeapStart();
    gpu_handle.ptr += incSize;
    _heightMapTexId = gpu_handle.ptr;
}

void DX12Sample::AdjustSizes()
{
    WINDOWINFO windowInfo = {};
    GetWindowInfo(m_hwnd, &windowInfo);
    LONG width = windowInfo.rcClient.right - windowInfo.rcClient.left;
    LONG height = windowInfo.rcClient.bottom - windowInfo.rcClient.top;

    if (m_width == width && m_height == height)
        return;

    m_width = width;
    m_height = height;
    // update window size
    _deviceResources->ClearSwapChainRts();
    ThrowIfFailed(_deviceResources->GetSwapChain()->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, NULL));
    auto swapChainRts = _RTManager->CreateRenderTargetsForSwapChain(_deviceResources->GetSwapChain());
    _deviceResources->SetSwapChainRts(swapChainRts);
    _sceneManager->UpdateWindowSize(m_width, m_height);
}

std::shared_ptr<DeviceResources> DX12Sample::CreateDeviceResources()
{
    std::shared_ptr<DeviceResources> resources;

    auto factory   = CreateDXGIFactory();
    auto device    = CreateDevice(factory);
    auto cmdQueue  = CreateCommandQueue(device);
    auto swapChain = CreateSwapChain(factory, cmdQueue, m_hwnd, _swapChainBuffersCount, m_width, m_height);
    resources      = std::make_shared<DeviceResources>(device, factory, swapChain, cmdQueue);
    return resources;
}

ComPtr<IDXGIFactory4> DX12Sample::CreateDXGIFactory()
{
    ComPtr<IDXGIFactory4> factory;
#ifdef _DEBUG
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
#else
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
#endif
    return factory;
}

ComPtr<ID3D12Device5> DX12Sample::CreateDevice(ComPtr<IDXGIFactory4> factory)
{
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);
    if (!hardwareAdapter)
        throw std::runtime_error("DirectX Raytracing is not supported by your OS, GPU and /or driver.");

    ComPtr<ID3D12Device5> device;
    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
    return device;
}

ComPtr<ID3D12CommandQueue> DX12Sample::CreateCommandQueue(ComPtr<ID3D12Device> device)
{
    // Create commandQueue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ComPtr<ID3D12CommandQueue> cmdQueue;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
    return cmdQueue;
}

ComPtr<IDXGISwapChain3> DX12Sample::CreateSwapChain(ComPtr<IDXGIFactory4>      factory,
                                                    ComPtr<ID3D12CommandQueue> cmdQueue,
                                                    HWND                       hwnd,
                                                    size_t                     buffersCount,
                                                    unsigned                   width,
                                                    unsigned                   height)
{
    // Create swapChain in factory using CmdQueue
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.BufferCount           = buffersCount;
    swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Flags                 = 0;
    swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Height                = height;
    swapChainDesc.Width                 = width;
    swapChainDesc.SampleDesc.Count      = 1;
    swapChainDesc.SampleDesc.Quality    = 0;
    swapChainDesc.Scaling               = DXGI_SCALING_STRETCH;
    swapChainDesc.Stereo                = FALSE;
    swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> tmp = nullptr;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(cmdQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &tmp));

    ComPtr<IDXGISwapChain3> swapChain;
    tmp->QueryInterface<IDXGISwapChain3>(&swapChain);
    return swapChain;
}

void DX12Sample::CreateObjects()
{
    auto lut = _colorsLut;
    // we do not need to gen colors for water
    lut.erase(40);
    lut.erase(50);
    lut[40] = XMUINT3{ 127, 127, 127 }; // rocky bottom

    TerrainManager tm{_worldGen, lut, 128};
    for (auto& chunk : tm.GetChunks())
    {
        auto obj = _sceneManager->CreateCustomObject(chunk.landVertices, chunk.landIndices, Material{MaterialType::Diffuse});
        obj->Position({(float)chunk.absX, (float)chunk.absY, 0.0});
        
        if (!chunk.waterVertices.empty() && !chunk.waterIndices.empty())
        {
            obj = _sceneManager->CreateCustomObject(chunk.waterVertices, chunk.waterIndices, Material{MaterialType::Water});
            obj->Position({(float)chunk.absX, (float)chunk.absY, 0.0});
        }
    }
}

void CalculateNormal(std::vector<GeometryVertex>& vertices, uint32_t index, int islandSize)
{
    auto calcNorm = [&vertices](uint32_t i1, uint32_t i2, uint32_t i3) {
        GeometryVertex& vertex1 = vertices[i1];
        const float3& position = vertex1.position;
        DirectX::FXMVECTOR pos1 = DirectX::FXMVECTOR({ position.x, position.y, position.z });

        GeometryVertex& vertex2 = vertices[i2];
        const float3& position2 = vertex2.position;
        DirectX::FXMVECTOR pos2 = DirectX::FXMVECTOR({ position2.x, position2.y, position2.z });

        GeometryVertex& vertex3 = vertices[i3];
        const float3& position3 = vertex3.position;
        DirectX::FXMVECTOR pos3 = DirectX::FXMVECTOR({ position3.x, position3.y, position3.z });

        XMVECTOR vector1 = DirectX::XMVectorSubtract(pos2, pos1);
        XMVECTOR vector2 = DirectX::XMVectorSubtract(pos3, pos1);

        XMVECTOR cross = DirectX::XMVector3Cross(vector1, vector2);
        DirectX::XMVECTOR norm = DirectX::XMVector3Normalize(cross);

        //XMVECTOR norm1  = DirectX::XMLoadFloat3(&vertex1.normal);
        //if (!DirectX::XMVector3Equal(norm, norm1) &&
        //    !DirectX::XMVector3Equal(DirectX::XMVector3Length(norm1), DirectX::XMVectorZero()))
        //{
        //    norm = DirectX::XMVectorAdd(norm1, norm);
        //    norm = DirectX::XMVector3Normalize(norm);
        //}
        DirectX::XMStoreFloat3(&vertex1.normal, norm);
        DirectX::XMStoreFloat3(&vertex2.normal, norm);
        DirectX::XMStoreFloat3(&vertex3.normal, norm);
    };

    if (vertices.size() > index + islandSize + 2)
    {
        calcNorm(index, index + 1, index + islandSize + 1);
        //calcNorm(index + islandSize + 2, index + islandSize + 1, index + 1);
    }
}

void DX12Sample::CreateIsland()
{
    int islandSize = _worldGen.GetSideSize() - 1;

    float multi = 300.0f;

    std::vector<GeometryVertex> vertices;
    vertices.reserve(islandSize * islandSize);

    float islandWidth = 6.0f;

    for (int y = 0; y <= islandSize; y++)
    {
        for (int x = 0; x <= islandSize; x++)
        {
            int height = _worldGen.GetHeight(x, y);

            const float nz     = height * 1.0f / multi;
            const float nx     = -islandWidth / 2 + islandWidth * ((float)x / islandSize);
            const float ny     = -islandWidth / 2 + islandWidth * ((float)y / islandSize);
            float3      normal = {0.0f, 0.0f, 0.0f};
            float3      color  = {0.5f, 0.5f, 0.5f};
            vertices.emplace_back(GeometryVertex{{nx, ny, nz}, normal, color});
        }
    }

    int                   colCnt = 0;
    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < islandSize * (islandSize - 1); ++i)
    {
        colCnt++;
        if (colCnt > islandSize)
        {
            colCnt = 0;
            continue;
        }

        CalculateNormal(vertices, i, islandSize);
        indices.emplace_back(i);
        indices.emplace_back(i + 1);
        indices.emplace_back(i + islandSize + 1);

        indices.emplace_back(i + islandSize + 2);
        indices.emplace_back(i + islandSize + 1);
        indices.emplace_back(i + 1);
    }

    _sceneManager->CreateCustomObject(vertices, indices, Material{MaterialType::Specular});
}
