set(SRC
    DX12Sample.cpp
    DX12Sample.h
    DXSample.cpp
    DXSample.h
    main.cpp
    SceneManager.cpp
    SceneManager.h
    stdafx.h

    Sample.hlsl # main library
)

set_source_files_properties(Sample.hlsl PROPERTIES VS_SHADER_MODEL "6.3" VS_SHADER_TYPE "Library" VS_SHADER_ENTRYPOINT "")

add_executable(dxr_sample ${SRC})

target_link_libraries(dxr_sample
    3rdparty
    utils
    winpixruntime
    dxgi.lib
    d3d12.lib
    d3dcompiler.lib)

set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
set_target_properties(dxr_sample PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

add_dependencies(dxr_sample copy_assets)
