set(SRC
    CommandList.cpp
    CommandList.h
    ComputePipelineState.cpp
    ComputePipelineState.h
    DescriptorHeap.cpp
    DescriptorHeap.h
    DXSampleHelper.h
    FeaturesCollector.h
    Math.h
    GraphicsPipelineState.cpp
    GraphicsPipelineState.h
    MeshManager.cpp
    MeshManager.h
    MeshObject.cpp
    MeshObject.h
    Misc.h
    RenderTargetManager.cpp
    RenderTargetManager.h
    RootSignature.cpp
    RootSignature.h
    SceneObject.cpp
    SceneObject.h
    ShaderTable.cpp
    ShaderTable.h
    SphericalCamera.cpp
    SphericalCamera.h
    Types.h
)

add_library(utils STATIC ${SRC})

target_link_libraries(utils dxgi.lib d3d12.lib d3dcompiler.lib d3dx12 shaders)

target_compile_definitions(utils PRIVATE WIN32_LEAN_AND_MEAN)
target_precompile_headers(utils PRIVATE stdafx.h)
