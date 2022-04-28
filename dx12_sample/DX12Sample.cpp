#include "stdafx.h"

#include "DX12Sample.h"

#include <utils/FeaturesCollector.h>
#include <utils/Math.h>
#include <utils/Shaders.h>

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>

using namespace std::chrono;

DX12Sample::DX12Sample(int windowWidth, int windowHeight, std::set<optTypes>& opts)
    : DXSample(windowWidth, windowHeight, L"HELLO YOPTA")
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

    _uiCmdList = std::make_unique<CommandList>(CommandListType::Direct, _deviceResources->GetDevice());
    _uiCmdList->Close();

    _RTManager    = std::make_unique<RenderTargetManager>(_deviceResources->GetDevice());
    auto swapChainRts = _RTManager->CreateRenderTargetsForSwapChain(_deviceResources->GetSwapChain());
    _deviceResources->SetSwapChainRts(swapChainRts);

    _sceneManager = std::make_unique<SceneManager>(_deviceResources, m_width, m_height, _cmdLineOpts, _RTManager.get());

    CreateObjects();

    D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
    imguiHeapDesc.NumDescriptors = 32;
    imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(_deviceResources->GetDevice()->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&_uiDescriptors)));

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

    _mouseSceneTracker.camPosition = _sceneManager->GetCamera().GetCameraPosition();
}

void DX12Sample::OnUpdate()
{
    static auto prevTime = high_resolution_clock::now();
    static int elapsedFrames = 0;
    static double elapsedTime = 0.0;

    auto diff = duration_cast<microseconds>(high_resolution_clock::now() - prevTime);
    double dt = (double)diff.count() / 1000000.0;

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

    //if (!_mouseSceneTracker.lBtnPressed)
    //{
    //    _sceneManager->GetCamera().SetRotation(_sceneManager->GetCamera().GetCameraPosition().rotation + dt * 10.0f );
    //    return;
    //}

    float dx = _mouseSceneTracker.camPosition.rotation + _mouseSceneTracker.pressedPoint.x - _mouseSceneTracker.currPoint.x;
    float dy = _mouseSceneTracker.camPosition.inclination - _mouseSceneTracker.pressedPoint.y + _mouseSceneTracker.currPoint.y;

    if (dy >= 89.0f)
        dy = 89.0f;
    else if (dy <= -89.0f)
        dy = -89.0f;

    _sceneManager->GetCamera().SetInclination(dy);
    _sceneManager->GetCamera().SetRotation(dx);
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
        ImGui::Text("X: %.3f", _sceneManager->GetCamera().GetEyePosition().x);
        ImGui::SameLine();
        ImGui::Text("Y: %.3f", _sceneManager->GetCamera().GetEyePosition().y);
        ImGui::SameLine();
        ImGui::Text("Z: %.3f", _sceneManager->GetCamera().GetEyePosition().z);
        ImGui::ColorEdit3("Ambient color", (float*)&ambientColor);
        ImGui::ColorEdit3("Light color", (float*)&lightColor);
        ImGui::SliderFloat3("Light direction", (float*)&lightDir, -1.0f, 1.0f);
        ImGui::End();

        _sceneManager->SetAmbientColor(ambientColor.x, ambientColor.y, ambientColor.z);
        _sceneManager->SetLightColor(lightColor.x, lightColor.y, lightColor.z);
        _sceneManager->SetLightDirection(lightDir.x, lightDir.y, lightDir.z);
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
}

bool DX12Sample::OnEvent(MSG msg)
{
    switch (msg.message)
    {
    case WM_KEYDOWN:
        if (msg.wParam == VK_ESCAPE)
            exit(0);
        break;
    case WM_LBUTTONDOWN:
        _mouseSceneTracker.lBtnPressed  = true;
        _mouseSceneTracker.pressedPoint = msg.pt;
        _mouseSceneTracker.currPoint    = msg.pt;
        _mouseSceneTracker.camPosition  = _sceneManager->GetCamera().GetCameraPosition();

        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, true);
        break;
    case WM_LBUTTONUP:
        _mouseSceneTracker.lBtnPressed = false;
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        break;
    case WM_MOUSEMOVE:
        if (_mouseSceneTracker.lBtnPressed)
            _mouseSceneTracker.currPoint = msg.pt;
        break;
    case WM_MOUSEWHEEL: {
        auto& camera = _sceneManager->GetCamera();
        float radius = camera.GetCameraPosition().radius;
        short zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
        camera.SetRadius(radius * (zDelta > 0 ? 0.9f : 1.1f));
        } break;
    }

    // DXSample does not check return value, so it will be false :)
    return false;
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
    }
}

void DX12Sample::DumpFeatures()
{
    FeaturesCollector collector {_deviceResources->GetDevice()};
    collector.CollectFeatures("deviceInfo.log");
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
    auto              cube     = _sceneManager->CreateCube();
    SpecularMaterial& specular = std::get<SpecularMaterial>(cube->GetMaterial().GetParams());
    specular.reflectance       = 350.0f;
    specular.color = {float(rand() % 50 + 50.0f) / 100, float(rand() % 50 + 50.0f) / 100, float(rand() % 50 + 50.0f) / 100};
    cube->Position({2.0, 0.0, 0.0});

    auto              island         = _sceneManager->CreateIslandCubes();
    SpecularMaterial& islandMaterial = std::get<SpecularMaterial>(island->GetMaterial().GetParams());
    islandMaterial.color             = {0.7f, 0.7f, 0.3f};
    islandMaterial.reflectance       = 500.0f;
    island->Position({0.0, 2.0, 0.0});

    auto             emptyCube = _sceneManager->CreateEmptyCube();
    DiffuseMaterial& diffuse   = std::get<DiffuseMaterial>(emptyCube->GetMaterial().GetParams());
    diffuse.color = {float(rand() % 50 + 50.0f) / 100, float(rand() % 50 + 50.0f) / 100, float(rand() % 50 + 50.0f) / 100};
    emptyCube->Rotation(0.5f);
}
