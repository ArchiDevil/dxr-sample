//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "DXSample.h"
#include <shellapi.h>

#include <functional>
#include <map>

std::map<HWND, std::function<void(HWND, UINT, WPARAM, LPARAM)>> globalHandlers;

DXSample::DXSample(UINT width, UINT height, std::wstring name) :
    m_width(width),
    m_height(height)
{
    m_title = name;

    WCHAR assetsPath[512];
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXSample::~DXSample()
{
    globalHandlers.erase(m_hwnd);
}

int DXSample::Run(HINSTANCE hInstance, int nCmdShow)
{
    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"WindowClass1";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    m_hwnd = CreateWindowEx(NULL,
        L"WindowClass1",
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        300,
        300,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,		// We have no parent window, NULL.
        NULL,		// We aren't using menus, NULL.
        hInstance,
        NULL);		// We aren't using multiple windows, NULL.

    globalHandlers[m_hwnd] = [this](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
        this->HandleWindowMessage(hwnd, message, wParam, lParam);
    };

    ShowWindow(m_hwnd, nCmdShow);

    // Initialize the sample. OnInit is defined in each child-implementation of DXSample.
    OnInit();

    // Main sample loop.
    MSG msg = { 0 };
    int returnCode = 0;
    while (true)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                returnCode = msg.wParam;
                break;
            }

            // Pass events into our sample.
            if (!OnEvent(msg))
            {
                returnCode = 0;
                break;
            }
        }

        OnUpdate();
        OnRender();
    }

    OnDestroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(returnCode);
}

// Returns bool whether the device supports DirectX Raytracing tier.
bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
{
    ComPtr<ID3D12Device> testDevice;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
        && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
        && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
void DXSample::GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter)
{
    IDXGIAdapter1* pAdapter = nullptr;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12 and DXR, but don't create the
        // actual device yet.
        if (IsDirectXRaytracingSupported(pAdapter))
            break;
    }

    *ppAdapter = pAdapter;
}

// Helper function for setting the window's title text.
void DXSample::SetCustomWindowText(LPCWSTR text)
{
    std::wstring windowText = m_title + L": " + text;
    SetWindowText(m_hwnd, windowText.c_str());
}

// Main message handler for the sample.
LRESULT CALLBACK DXSample::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Handle destroy/shutdown messages.
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    if (auto iter = globalHandlers.find(hWnd); iter != globalHandlers.end())
        iter->second(hWnd, message, wParam, lParam);

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}
