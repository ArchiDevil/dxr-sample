#include "stdafx.h"
#include "DX12Sample.h"

#include <backends/imgui_impl_win32.h>

#include <shellapi.h>
#include <iostream>

#ifdef _DEBUG
int main(int argc, char * argv[])
#else // _DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
    HINSTANCE localInstance = GetModuleHandle(0);

    ImGui_ImplWin32_EnableDpiAwareness();

#ifndef _DEBUG
    int argc = 0;
    wchar_t ** argv = CommandLineToArgvW(GetCommandLine(), &argc);
#endif

    const std::map<std::wstring, optTypes> argumentToString = {};

    std::set<optTypes> arguments {};
    for (int i = 1; i < argc; ++i)
    {
#ifdef _DEBUG
        auto iter = argumentToString.find(std::wstring(argv[i], argv[i] + strlen(argv[i])));
#else
        auto iter = argumentToString.find(argv[i]);
#endif
        if (iter != argumentToString.end())
            arguments.insert(iter->second);
        else
        {
            std::wcout << L"Usage: dxr_sample <opts>" << std::endl;
            std::wcout << L"where <opts>:" << std::endl;
            for (auto & pair : argumentToString)
            {
                std::wcout << L"\t" << pair.first << std::endl;
            }

            return -1;
        }
    }

    srand(time(nullptr));

    try
    {
        DX12Sample app = {1280, 720, arguments};
        return app.Run(localInstance, SW_SHOW);
    }
    catch (const std::exception& e)
    {
        MessageBoxA(NULL, e.what(), "Exception", NULL);
        return 1;
    }
}
