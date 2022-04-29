set(SRC
    DeviceResources.h
    DX12Sample.cpp
    DX12Sample.h
    DXSample.cpp
    DXSample.h
    main.cpp
    SceneManager.cpp
    SceneManager.h
    stdafx.h
    
    worldgen/Noise.cpp
    worldgen/Noise.h
    worldgen/WorldGen.cpp
    worldgen/WorldGen.h
)

add_executable(dxr_sample ${SRC})

target_link_libraries(dxr_sample
    d3dx12
    utils
    imgui
    shaders
    winpixruntime
    dxgi.lib
    d3d12.lib
    d3dcompiler.lib)

target_compile_definitions(dxr_sample PRIVATE WIN32_LEAN_AND_MEAN)
target_precompile_headers(dxr_sample PRIVATE stdafx.h)

set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
