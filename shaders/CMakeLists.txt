set(FILES
    Dummy.cpp
    Common.h # HLSL <-> C++ translation
    Sample.hlsl # main library
    Shadows.hlsl
)

add_library(shaders STATIC ${FILES})

set_source_files_properties(Sample.hlsl Shadows.hlsl
    PROPERTIES 
        VS_SHADER_MODEL "6.3"
        VS_SHADER_TYPE "Library"
        VS_SHADER_ENTRYPOINT ""
        VS_SHADER_FLAGS "/Zpr /WX" # for row-major matrices
)

add_custom_command(TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/shaders/${CMAKE_CFG_INTDIR}/Sample.cso"
        "${CMAKE_BINARY_DIR}/dx12_sample/Sample.cso"
)

add_custom_command(TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/shaders/${CMAKE_CFG_INTDIR}/Sample.cso"
        "${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/Sample.cso"
)

add_custom_command(TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/shaders/${CMAKE_CFG_INTDIR}/Shadows.cso"
        "${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/Shadows.cso"
)

add_custom_command(TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/shaders/${CMAKE_CFG_INTDIR}/Shadows.cso"
        "${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/Shadows.cso"
)
