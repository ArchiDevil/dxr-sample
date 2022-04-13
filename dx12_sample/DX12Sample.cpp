#include "stdafx.h"

#include "DX12Sample.h"

#include <utils/Math.h>
#include <utils/Shaders.h>
#include <utils/FeaturesCollector.h>

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

    CreateDXGIFactory();
    CreateDevice();
    DumpFeatures();
    CreateCommandQueue();
    CreateSwapChain();

    _RTManager.reset(new RenderTargetManager(_device));

    _sceneManager.reset(new SceneManager(_device,
                                         m_width,
                                         m_height,
                                         _cmdLineOpts,
                                         _cmdQueue,
                                         _swapChain,
                                         _RTManager.get()));
}

void DX12Sample::OnUpdate()
{
    static auto prevTime = high_resolution_clock::now();
    static int elapsedFrames = 0;
    static double elapsedTime = 0.0;

    microseconds diff = duration_cast<microseconds>(high_resolution_clock::now() - prevTime);
    double dt = (double)diff.count() / 1000000.0;

    prevTime = high_resolution_clock::now();
    elapsedFrames++;
    elapsedTime += dt;

    if (elapsedTime > 1.0)
    {
        std::wostringstream ss;
        ss << "FPS: " << elapsedFrames;
        SetWindowText(m_hwnd, ss.str().c_str());
        elapsedFrames = 0;
        elapsedTime -= 1.0;
    }

    static double time = 0;
    time += dt;

    _sceneManager->GetCamera().SetInclination(time * 3.0f);
    _sceneManager->GetCamera().SetRotation(time * 3.0f);
}

void DX12Sample::OnRender()
{
    _sceneManager->DrawAll();
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
    }

    // DXSample does not check return value, so it will be false :)
    return false;
}

void DX12Sample::DumpFeatures()
{
    FeaturesCollector collector {_device};
    collector.CollectFeatures("deviceInfo.log");
}

void DX12Sample::CreateDXGIFactory()
{
#ifdef _DEBUG
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_DXFactory)));
#else
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&_DXFactory)));
#endif
}

void DX12Sample::CreateDevice()
{
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(_DXFactory.Get(), &hardwareAdapter);
    if (!hardwareAdapter)
        throw std::runtime_error("DirectX Raytracing is not supported by your OS, GPU and /or driver.");
    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));
}

void DX12Sample::CreateCommandQueue()
{
    // Create commandQueue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue)));
}

void DX12Sample::CreateSwapChain()
{
    // Create swapChain in factory using CmdQueue
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.BufferCount = _swapChainBuffersCount;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Flags = 0;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Height = m_height;
    swapChainDesc.Width = m_width;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> pTmpSwapChain = nullptr;
    ThrowIfFailed(_DXFactory->CreateSwapChainForHwnd(_cmdQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &pTmpSwapChain));
    pTmpSwapChain.Get()->QueryInterface<IDXGISwapChain3>(&_swapChain);
}
